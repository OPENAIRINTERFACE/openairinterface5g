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


#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <math.h>
#include "common_lib.h"
#include "msc.h"


#include "defs_common.h"
#include "impl_defs_top.h"
#include "PHY/TOOLS/time_meas.h"
//#include "PHY/CODING/coding_defs.h"
#include "PHY/TOOLS/tools_defs.h"
#include "platform_types.h"
#include "PHY/LTE_TRANSPORT/transport_common.h"
#include "PHY/LTE_TRANSPORT/transport_eNB.h"
#include <pthread.h>


#include "openair2/PHY_INTERFACE/IF_Module.h"


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
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
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
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
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
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// \internal This variable is protected by \ref mutex_prach.
  int instance_cnt_prach_br;
#endif
  /// \internal This variable is protected by \ref mutex_synch.
  int instance_cnt_synch;
  /// \internal This variable is protected by \ref mutex_eNBs.
  int instance_cnt_eNBs;
  /// \brief Instance count for rx processing thread.
  /// \internal This variable is protected by \ref mutex_asynch_rxtx.
  int instance_cnt_asynch_rxtx;
  /// \internal This variable is protected by \ref mutex_fep
  int instance_cnt_fep;
  /// \internal This variable is protected by \ref mutex_feptx
  int instance_cnt_feptx;

  /// \internal This variable is protected by \ref mutex_ru_thread
  int instance_cnt_ru;
  /// pthread structure for RU FH processing thread
  pthread_t pthread_FH;
  /// pthread structure for RU control thread
  pthread_t pthread_ctrl;

  /// This varible is protected by \ref mutex_emulatedRF
  int instance_cnt_emulateRF;
  /// pthread structure for RU FH processing thread

  pthread_t pthread_FH1;

  /// pthread structure for RU prach processing thread
  pthread_t pthread_prach;
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
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

  /// pthread attributes for RU control thread
  pthread_attr_t attr_ctrl;

  pthread_attr_t attr_FH1;

  /// pthread attributes for RU prach
  pthread_attr_t attr_prach;
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
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
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
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
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// condition variable for RU prach thread BL/CE UEs
  pthread_cond_t cond_prach_br;
#endif
  /// condition variable for RU synch thread
  pthread_cond_t cond_synch;
  /// condition variable for asynch RX/TX thread
  pthread_cond_t cond_asynch_rxtx;
  /// condition variable for RU RX FEP thread
  pthread_cond_t cond_fep;
  /// condition variable for RU TX FEP thread
  pthread_cond_t cond_feptx;
  /// condition variable for emulated RF
  pthread_cond_t cond_emulateRF;
  /// condition variable for eNB signal
  pthread_cond_t cond_eNBs;
  /// condition variable for ru_thread
  pthread_cond_t cond_ru_thread;
  /// mutex for RU FH
  pthread_mutex_t mutex_FH;
  pthread_mutex_t mutex_FH1;
  /// mutex for RU prach
  pthread_mutex_t mutex_prach;
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// mutex for RU prach BL/CE UEs
  pthread_mutex_t mutex_prach_br;
#endif
  /// mutex for RU synch
  pthread_mutex_t mutex_synch;
  /// mutex for eNB signal
  pthread_mutex_t mutex_eNBs;
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
  int emulate_rf_busy;
} RU_proc_t;

typedef enum {
  LOCAL_RF        =0,
  REMOTE_IF5      =1,
  REMOTE_MBP_IF5  =2,
  REMOTE_IF4p5    =3,
  REMOTE_IF1pp    =4,
  MAX_RU_IF_TYPES =5
} RU_if_south_t;

typedef enum {
  RU_IDLE   = 0,
  RU_CONFIG = 1,
  RU_READY  = 2,
  RU_RUN    = 3,
  RU_ERROR  = 4,
  RU_SYNC   = 5,
  RU_CHECK_SYNC = 6
} rru_state_t;

/// Some commamds to RRU. Not sure we should do it like this !
typedef enum {
  EMPTY     = 0,
  STOP_RU   = 1,
  RU_FRAME_RESYNCH = 2,
  WAIT_RESYNCH = 3
} rru_cmd_t;

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
  LTE_DL_FRAME_PARMS frame_parms;
  ///timing offset used in TDD
  int              N_TA_offset; 
  /// SF extension used in TDD (unit: number of samples at 30.72MHz) (this is an expert option)
  int              sf_extension;
  /// "end of burst delay" used in TDD (unit: number of samples at 30.72MHz) (this is an expert option)
  int              end_of_burst_delay;
  /// RF device descriptor
  openair0_device rfdevice;
  /// HW configuration
  openair0_config_t openair0_cfg;
  /// Number of eNBs using this RU
  int num_eNB;
  /// list of eNBs using this RU
  struct PHY_VARS_eNB_s *eNB_list[NUMBER_OF_eNB_MAX];
  /// Mapping of antenna ports to RF chain index
  openair0_rf_map      rf_map;
  /// IF device descriptor
  openair0_device ifdevice;
  /// Pointer for ifdevice buffer struct
  if_buffer_t ifbuffer;
  /// if prach processing is to be performed in RU
  int                  do_prach;
  /// function pointer to synchronous RX fronthaul function (RRU,3GPP_eNB)
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
  /// function pointer to wakeup routine in lte-enb.
  int (*wakeup_rxtx)(struct PHY_VARS_eNB_s *eNB, struct RU_t_s *ru);
  /// function pointer to wakeup routine in lte-enb.
  void (*wakeup_prach_eNB)(struct PHY_VARS_eNB_s *eNB,struct RU_t_s *ru,int frame,int subframe);
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// function pointer to wakeup routine in lte-enb.
  void (*wakeup_prach_eNB_br)(struct PHY_VARS_eNB_s *eNB,struct RU_t_s *ru,int frame,int subframe);
#endif
  /// function pointer to eNB entry routine
  void (*eNB_top)(struct PHY_VARS_eNB_s *eNB, int frame_rx, int subframe_rx, char *string, struct RU_t_s *ru);
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
  RU_CALIBRATION calibration; 
  /// beamforming weight vectors per eNB
  int32_t **beam_weights[NUMBER_OF_eNB_MAX+1][15];

  /// received frequency-domain signal for PRACH (IF4p5 RRU) 
  int16_t              **prach_rxsigF;
  /// received frequency-domain signal for PRACH BR (IF4p5 RRU) 
  int16_t              **prach_rxsigF_br[4];
  /// sequence number for IF5
  uint8_t seqno;
  /// initial timestamp used as an offset make first real timestamp 0
  openair0_timestamp   ts_offset;
  /// Current state of the RU
  rru_state_t state;
  /// Command to do
  rru_cmd_t cmd;
  /// value to be passed using command
  uint16_t cmdval;
  /// process scheduling variables
  RU_proc_t            proc;
  /// stats thread pthread descriptor
  pthread_t            ru_stats_thread;
  /// OTA synchronization signal
  int16_t *dmrssync;
  /// OTA synchronization correlator output
  uint64_t *dmrs_corr;
  /// sleep time in us for delaying L1 wakeup 
  int wakeup_L1_sleeptime;
  /// maximum number of sleeps
  int wakeup_L1_sleep_cnt_max;
} RU_t;



#define MAX_RRU_CONFIG_SIZE 1024
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
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  int emtc_prach_CElevel_enable[MAX_BANDS_PER_RRU][4];
  /// emtc_prach_FreqOffset for IF4p5 per CE Level
  int emtc_prach_FreqOffset[MAX_BANDS_PER_RRU][4];
  /// emtc_prach_ConfigIndex for IF4p5 per CE Level
  int emtc_prach_ConfigIndex[MAX_BANDS_PER_RRU][4];
#endif
  /// mutex for asynch RX/TX thread
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
  /// measured RX power based on DRS
  int ulsch_power[2];
  /// \brief llr values.
  /// - first index: ? [0..1179743] (hard coded)
  int16_t *llr;
} LTE_eNB_PUSCH;

#define PBCH_A 24
typedef struct {
  uint8_t pbch_d[96+(3*(16+PBCH_A))];
  uint8_t pbch_w[3*3*(16+PBCH_A)];
  uint8_t pbch_e[1920];
} LTE_eNB_PBCH;

#define MAX_NUM_RX_PRACH_PREAMBLES 4

typedef struct {
  /// \brief ?.
  /// first index: ? [0..1023] (hard coded)
  int16_t *prachF;
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
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// indicator of first frame in a group of PRACH repetitions
  int first_frame[4];
  /// current repetition for each CE level
  int repetition_number[4];
#endif
} LTE_eNB_PRACH;

#include "PHY/TOOLS/time_meas.h"
#include "PHY/CODING/coding_defs.h"
#include "PHY/TOOLS/tools_defs.h"
#include "PHY/LTE_TRANSPORT/transport_eNB.h"

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
} L1_rxtx_proc_t;

typedef struct {
  struct PHY_VARS_eNB_s *eNB;
  int UE_id;
  int harq_pid;
  int llr8_flag;
  int ret;
} td_params;

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
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// subframe to act upon for reception of prach BL/CE UEs
  int subframe_prach_br;
#endif
  /// frame to act upon for reception
  int frame_rx;
  /// frame to act upon for transmission
  int frame_tx;
  /// frame to act upon for PRACH
  int frame_prach;
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// frame to act upon for PRACH BL/CE UEs
  int frame_prach_br;
#endif
  /// \internal This variable is protected by \ref mutex_td.
  int instance_cnt_td;
  /// \internal This variable is protected by \ref mutex_te.
  int instance_cnt_te;
  /// \internal This variable is protected by \ref mutex_prach.
  int instance_cnt_prach;
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// \internal This variable is protected by \ref mutex_prach for BL/CE UEs.
  int instance_cnt_prach_br;
#endif
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
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// pthread attributes for prach processing thread BL/CE UEs
  pthread_attr_t attr_prach_br;
#endif
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
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// scheduling parameters for prach thread
  struct sched_param sched_param_prach_br;
#endif
  /// scheduling parameters for asynch_rxtx thread
  struct sched_param sched_param_asynch_rxtx;
  /// pthread structure for parallel turbo-decoder thread
  pthread_t pthread_td;
  /// pthread structure for parallel turbo-encoder thread
  pthread_t pthread_te;
  /// pthread structure for PRACH thread
  pthread_t pthread_prach;
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// pthread structure for PRACH thread BL/CE UEs
  pthread_t pthread_prach_br;
#endif
  /// condition variable for parallel turbo-decoder thread
  pthread_cond_t cond_td;
  /// condition variable for parallel turbo-encoder thread
  pthread_cond_t cond_te;
  /// condition variable for PRACH processing thread;
  pthread_cond_t cond_prach;
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// condition variable for PRACH processing thread BL/CE UEs;
  pthread_cond_t cond_prach_br;
#endif

  /// condition variable for asynch RX/TX thread
  pthread_cond_t cond_asynch_rxtx;
  /// mutex for parallel turbo-decoder thread
  pthread_mutex_t mutex_td;
  /// mutex for parallel turbo-encoder thread
  pthread_mutex_t mutex_te;
  /// mutex for PRACH thread
  pthread_mutex_t mutex_prach;
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// mutex for PRACH thread for BL/CE UEs
  pthread_mutex_t mutex_prach_br;
#endif
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
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// mask for RUs serving eNB (PRACH)
  int RU_mask_prach_br;
#endif
  /// parameters for turbo-decoding worker thread
  td_params tdp;
  /// parameters for turbo-encoding worker thread
  te_params tep[3];
  /// set of scheduling variables RXn-TXnp4 threads
  L1_rxtx_proc_t L1_proc,L1_proc_tx;
  /// stats thread pthread descriptor
  pthread_t process_stats_thread;
  /// for waking up tx procedure
  RU_proc_t *ru_proc;
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
  // eNB measurements (per user)
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
} PHY_MEASUREMENTS_eNB;


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
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// NFAPI PRACH information BL/CE UEs
  nfapi_preamble_pdu_t preamble_list_br[MAX_NUM_RX_PRACH_PREAMBLES];
#endif
  Sched_Rsp_t          Sched_INFO;
  LTE_eNB_PDCCH        pdcch_vars[2];
  LTE_eNB_PHICH        phich_vars[2];
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  LTE_eNB_EPDCCH       epdcch_vars[2];
  LTE_eNB_MPDCCH       mpdcch_vars[2];
  LTE_eNB_PRACH        prach_vars_br;
#endif
  LTE_eNB_COMMON       common_vars;
  LTE_eNB_UCI          uci_vars[NUMBER_OF_UCI_VARS_MAX];
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

  /// cell-specific reference symbols
  uint32_t         lte_gold_table[20][2][14];

  /// UE-specific reference symbols (p=5), TM 7
  uint32_t         lte_gold_uespec_port5_table[NUMBER_OF_UE_MAX][20][38];

  /// UE-specific reference symbols (p=7...14), TM 8/9/10
  uint32_t         lte_gold_uespec_table[2][20][2][21];

  /// mbsfn reference symbols
  uint32_t         lte_gold_mbsfn_table[10][3][42];
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// mbsfn reference symbols
  uint32_t         lte_gold_mbsfn_khz_1dot25_table[10][150];
#endif

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
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  uint32_t X_u_br[4][64][839];
#endif
  uint8_t pbch_configured;
  uint8_t pbch_pdu[4]; //PBCH_PDU_SIZE
  char eNB_generate_rar;

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
  time_stats_t dlsch_turbo_encoding_waiting_stats;
  time_stats_t dlsch_turbo_encoding_signal_stats;
  time_stats_t dlsch_turbo_encoding_main_stats;
  time_stats_t dlsch_turbo_encoding_wakeup_stats0;
  time_stats_t dlsch_turbo_encoding_wakeup_stats1;
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
} PHY_VARS_eNB;


#endif /* __PHY_DEFS_ENB__H__ */

