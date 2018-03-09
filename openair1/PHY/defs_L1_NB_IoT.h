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
#ifndef __PHY_DEFS_NB_IOT__H__
#define __PHY_DEFS_NB_IOT__H__

#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <math.h>
#include "common_lib.h"
#include "openair2/PHY_INTERFACE/IF_Module_NB_IoT.h"

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
//******************************************************************************************************
/*
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

*/


// #define PAGE_MASK 0xfffff000
// #define virt_to_phys(x) (x)

// #define openair_sched_exit() exit(-1)


// #define max(a,b)  ((a)>(b) ? (a) : (b))
// #define min(a,b)  ((a)<(b) ? (a) : (b))


// #define bzero(s,n) (memset((s),0,(n)))

// #define cmax(a,b)  ((a>b) ? (a) : (b))
// #define cmin(a,b)  ((a<b) ? (a) : (b))

// #define cmax3(a,b,c) ((cmax(a,b)>c) ? (cmax(a,b)) : (c))

// /// suppress compiler warning for unused arguments
// #define UNUSED(x) (void)x;


#include "PHY/impl_defs_top_NB_IoT.h"
//#include "impl_defs_top.h"
//#include "impl_defs_lte.h"
#include "PHY/impl_defs_lte_NB_IoT.h"

#include "PHY/TOOLS/time_meas.h"
//#include "PHY/CODING/defs.h"
#include "PHY/CODING/defs_NB_IoT.h"
#include "openair2/PHY_INTERFACE/IF_Module_NB_IoT.h"
//#include "PHY/TOOLS/defs.h"
//#include "platform_types.h"
///#include "openair1/PHY/LTE_TRANSPORT/defs_nb_iot.h"

////////////////////////////////////////////////////////////////////#ifdef OPENAIR_LTE    (check if this is required)

//#include "PHY/LTE_TRANSPORT/defs.h"
#include "PHY/LTE_TRANSPORT/defs_NB_IoT.h"
#include <pthread.h>

#include "targets/ARCH/COMMON/common_lib.h"
#include "targets/COMMON/openairinterface5g_limits.h"

#define NUM_DCI_MAX_NB_IoT 32

#define NUMBER_OF_eNB_SECTORS_MAX_NB_IoT 3

#define NB_BANDS_MAX_NB_IoT 8


#ifdef OCP_FRAMEWORK
#include <enums.h>
#else
typedef enum {normal_txrx_NB_IoT=0,rx_calib_ue_NB_IoT=1,rx_calib_ue_med_NB_IoT=2,rx_calib_ue_byp_NB_IoT=3,debug_prach_NB_IoT=4,no_L2_connect_NB_IoT=5,calib_prach_tx_NB_IoT=6,rx_dump_frame_NB_IoT=7,loop_through_memory_NB_IoT=8} runmode_NB_IoT_t;
#endif
/*
enum transmission_access_mode {
  NO_ACCESS=0,
  POSTPONED_ACCESS,
  CANCELED_ACCESS,
  UNKNOWN_ACCESS,
  SCHEDULED_ACCESS,
  CBA_ACCESS};

typedef enum  {
  eNodeB_3GPP=0,   // classical eNodeB function
  eNodeB_3GPP_BBU, // eNodeB with NGFI IF5
  NGFI_RCC_IF4p5,  // NGFI_RCC (NGFI radio cloud center)
  NGFI_RAU_IF4p5,
  NGFI_RRU_IF5,    // NGFI_RRU (NGFI remote radio-unit,IF5)
  NGFI_RRU_IF4p5   // NGFI_RRU (NGFI remote radio-unit,IF4p5)
} eNB_func_t;

typedef enum {
  synch_to_ext_device=0,  // synch to RF or Ethernet device
  synch_to_other          // synch to another source (timer, other CC_id)
} eNB_timing_t;
#endif
*/
typedef struct UE_SCAN_INFO_NB_IoT_s {
  /// 10 best amplitudes (linear) for each pss signals
  int32_t             amp[3][10];
  /// 10 frequency offsets (kHz) corresponding to best amplitudes, with respect do minimum DL frequency in the band
  int32_t             freq_offset_Hz[3][10];

} UE_SCAN_INFO_NB_IoT_t;

/// Top-level PHY Data Structure for RN
typedef struct {
  /// Module ID indicator for this instance
  uint8_t             Mod_id;
  uint32_t            frame;
  // phy_vars_eNB_NB_IoT
  // phy_vars ue
  // cuurently only used to store and forward the PMCH
  uint8_t             mch_avtive[10];
  uint8_t             sync_area[10]; // num SF
  NB_IoT_UE_DLSCH_t   *dlsch_rn_MCH[10];

} PHY_VARS_RN_NB_IoT;
/*
#ifdef OCP_FRAMEWORK
#include <enums.h>
#else
//typedef enum {normal_txrx=0,rx_calib_ue=1,rx_calib_ue_med=2,rx_calib_ue_byp=3,debug_prach=4,no_L2_connect=5,calib_prach_tx=6,rx_dump_frame=7,loop_through_memory=8} runmode_t;
*/
// enum transmission_access_mode {
//   NO_ACCESS=0,
//   POSTPONED_ACCESS,
//   CANCELED_ACCESS,
//   UNKNOWN_ACCESS,
//   SCHEDULED_ACCESS,
//   CBA_ACCESS};

typedef enum  {
  eNodeB_3GPP_NB_IoT=0,   // classical eNodeB function
  eNodeB_3GPP_BBU_NB_IoT, // eNodeB with NGFI IF5
  NGFI_RCC_IF4p5_NB_IoT,  // NGFI_RCC (NGFI radio cloud center)
  NGFI_RAU_IF4p5_NB_IoT,
  NGFI_RRU_IF5_NB_IoT,    // NGFI_RRU (NGFI remote radio-unit,IF5)
  NGFI_RRU_IF4p5_NB_IoT   // NGFI_RRU (NGFI remote radio-unit,IF4p5)
} eNB_func_NB_IoT_t;

typedef enum {

  synch_to_ext_device_NB_IoT=0,  // synch to RF or Ethernet device
  synch_to_other_NB_IoT          // synch to another source (timer, other CC_id)

} eNB_timing_NB_IoT_t;
////////////////////////////////////////////////////////////////////#endif


typedef struct {

  struct                        PHY_VARS_eNB_NB_IoT_s       *eNB;
  NB_IoT_eNB_NDLSCH_t           *dlsch;
  int                           G;

} te_params_NB_IoT;


typedef struct {

  struct                        PHY_VARS_eNB_NB_IoT_s       *eNB;
  int                           UE_id;
  int                           harq_pid;
  int                           llr8_flag;
  int                           ret;

} td_params_NB_IoT;


/// Context data structure for RX/TX portion of subframe processing
typedef struct {
  /// timestamp transmitted to HW
  openair0_timestamp    timestamp_tx;
  /// subframe to act upon for transmission
  int                   subframe_tx;
  /// subframe to act upon for reception
  int                   subframe_rx;
  /// frame to act upon for transmission
  int                   frame_tx;
  /// frame to act upon for reception
  int                   frame_rx;
  /// \brief Instance count for RXn-TXnp4 processing thread.
  /// \internal This variable is protected by \ref mutex_rxtx.
  int                   instance_cnt_rxtx;
  /// pthread structure for RXn-TXnp4 processing thread
  pthread_t             pthread_rxtx;
  /// pthread attributes for RXn-TXnp4 processing thread
  pthread_attr_t        attr_rxtx;
  /// condition variable for tx processing thread
  pthread_cond_t        cond_rxtx;
  /// mutex for RXn-TXnp4 processing thread
  pthread_mutex_t       mutex_rxtx;
  /// scheduling parameters for RXn-TXnp4 thread
  struct                sched_param sched_param_rxtx;
  /// NB-IoT for IF_Module
  pthread_t             pthread_l2;
  pthread_cond_t        cond_l2;
  pthread_mutex_t       mutex_l2;
  int                   instance_cnt_l2;
  pthread_attr_t        attr_l2;

} eNB_rxtx_proc_NB_IoT_t;
/*
typedef struct {
  struct PHY_VARS_eNB_NB_IoT_s *eNB;
  int UE_id;
  int harq_pid;
  int llr8_flag;
  int ret;
} td_params;

typedef struct {
  struct PHY_VARS_eNB_NB_IoT_s *eNB;
  LTE_eNB_DLSCH_t *dlsch;
  int G;
} te_params;
*/

/// Context data structure for eNB subframe processing
typedef struct eNB_proc_NB_IoT_t_s {
  /// thread index
  int                     thread_index;
  /// timestamp received from HW
  openair0_timestamp      timestamp_rx;
  /// timestamp to send to "slave rru"
  openair0_timestamp      timestamp_tx;
  /// subframe to act upon for reception
  int                     subframe_rx;
  /// symbol mask for IF4p5 reception per subframe
  uint32_t                symbol_mask[10];
  /// subframe to act upon for PRACH
  int                     subframe_prach;
  /// frame to act upon for reception
  int                     frame_rx;
  /// frame to act upon for transmission
  int                     frame_tx;
  /// frame offset for secondary eNBs (to correct for frame asynchronism at startup)
  int                     frame_offset;
  /// frame to act upon for PRACH
  int                     frame_prach;
  /// \internal This variable is protected by \ref mutex_fep.
  int                     instance_cnt_fep;
  /// \internal This variable is protected by \ref mutex_td.
  int                     instance_cnt_td;
  /// \internal This variable is protected by \ref mutex_te.
  int                     instance_cnt_te;
  /// \brief Instance count for FH processing thread.
  /// \internal This variable is protected by \ref mutex_FH.
  int                     instance_cnt_FH;
  /// \brief Instance count for rx processing thread.
  /// \internal This variable is protected by \ref mutex_prach.
  int                     instance_cnt_prach;
  // instance count for over-the-air eNB synchronization
  int                     instance_cnt_synch;
  /// \internal This variable is protected by \ref mutex_asynch_rxtx.
  int                     instance_cnt_asynch_rxtx;
  /// pthread structure for FH processing thread
  pthread_t               pthread_FH;
  /// pthread structure for eNB single processing thread
  pthread_t               pthread_single;
  /// pthread structure for asychronous RX/TX processing thread
  pthread_t               pthread_asynch_rxtx;
  /// flag to indicate first RX acquisition
  int                     first_rx;
  /// flag to indicate first TX transmission
  int                     first_tx;
  /// pthread attributes for parallel fep thread
  pthread_attr_t          attr_fep;
  /// pthread attributes for parallel turbo-decoder thread
  pthread_attr_t          attr_td;
  /// pthread attributes for parallel turbo-encoder thread
  pthread_attr_t          attr_te;
  /// pthread attributes for FH processing thread
  pthread_attr_t          attr_FH;
  /// pthread attributes for single eNB processing thread
  pthread_attr_t          attr_single;
  /// pthread attributes for prach processing thread
  pthread_attr_t          attr_prach;
  /// pthread attributes for over-the-air synch thread
  pthread_attr_t          attr_synch;
  /// pthread attributes for asynchronous RX thread
  pthread_attr_t          attr_asynch_rxtx;
  /// scheduling parameters for parallel fep thread
  struct                  sched_param sched_param_fep;
  /// scheduling parameters for parallel turbo-decoder thread
  struct                  sched_param sched_param_td;
  /// scheduling parameters for parallel turbo-encoder thread
  struct                  sched_param sched_param_te;
  /// scheduling parameters for FH thread
  struct                  sched_param sched_param_FH;
  /// scheduling parameters for single eNB thread
  struct                  sched_param sched_param_single;
  /// scheduling parameters for prach thread
  struct                  sched_param sched_param_prach;
  /// scheduling parameters for over-the-air synchronization thread
  struct                  sched_param sched_param_synch;
  /// scheduling parameters for asynch_rxtx thread
  struct                  sched_param sched_param_asynch_rxtx;
  /// pthread structure for parallel fep thread
  pthread_t               pthread_fep;
  /// pthread structure for parallel turbo-decoder thread
  pthread_t               pthread_td;
  /// pthread structure for parallel turbo-encoder thread
  pthread_t               pthread_te;
  /// pthread structure for PRACH thread
  pthread_t               pthread_prach;
  /// pthread structure for eNB synch thread
  pthread_t               pthread_synch;
  /// condition variable for parallel fep thread
  pthread_cond_t          cond_fep;
  /// condition variable for parallel turbo-decoder thread
  pthread_cond_t          cond_td;
  /// condition variable for parallel turbo-encoder thread
  pthread_cond_t          cond_te;
  /// condition variable for FH thread
  pthread_cond_t          cond_FH;
  /// condition variable for PRACH processing thread;
  pthread_cond_t          cond_prach;
  // condition variable for over-the-air eNB synchronization
  pthread_cond_t          cond_synch;
  /// condition variable for asynch RX/TX thread
  pthread_cond_t          cond_asynch_rxtx;
  /// mutex for RU access to  processing (NPDSCH/PUSCH)
  pthread_mutex_t mutex_RU;
  /// mutex for parallel fep thread
  pthread_mutex_t         mutex_fep;
  /// mutex for parallel turbo-decoder thread
  pthread_mutex_t         mutex_td;
  /// mutex for parallel turbo-encoder thread
  pthread_mutex_t         mutex_te;
  /// mutex for FH
  pthread_mutex_t         mutex_FH;
  /// mutex for PRACH thread
  pthread_mutex_t         mutex_prach;
  // mutex for over-the-air eNB synchronization
  pthread_mutex_t         mutex_synch;
  /// mutex for RU access to NB-IoT processing (NPRACH)
  pthread_mutex_t mutex_RU_PRACH;
  /// mutex for asynch RX/TX thread
  pthread_mutex_t         mutex_asynch_rxtx;
  /// mask for RUs serving nbiot  (NPDSCH/NPUSCH)
  int RU_mask;
  /// mask for RUs serving nbiot (PRACH)
  int RU_mask_prach;
#ifdef Rel14
  /// mask for RUs serving eNB (PRACH)
  int RU_mask_prach_br;
#endif
  /// parameters for turbo-decoding worker thread
  td_params_NB_IoT        tdp;
  /// parameters for turbo-encoding worker thread
  te_params_NB_IoT        tep;
  /// number of slave threads
  int                     num_slaves;
  /// array of pointers to slaves
  struct                  eNB_proc_NB_IoT_t_s           **slave_proc;
  /// set of scheduling variables RXn-TXnp4 threads
  // newly added for NB_IoT
  eNB_rxtx_proc_NB_IoT_t  proc_rxtx[2];

} eNB_proc_NB_IoT_t;


/// Context data structure for RX/TX portion of subframe processing
typedef struct {
  /// index of the current UE RX/TX proc
  int                   proc_id;
  /// timestamp transmitted to HW
  openair0_timestamp    timestamp_tx;
  /// subframe to act upon for transmission
  int                   subframe_tx;
  /// subframe to act upon for reception
  int                   subframe_rx;
  /// frame to act upon for transmission
  int                   frame_tx;
  /// frame to act upon for reception
  int                   frame_rx;
  /// \brief Instance count for RXn-TXnp4 processing thread.
  /// \internal This variable is protected by \ref mutex_rxtx.
  int                   instance_cnt_rxtx;
  /// pthread structure for RXn-TXnp4 processing thread
  pthread_t             pthread_rxtx;
  /// pthread attributes for RXn-TXnp4 processing thread
  pthread_attr_t        attr_rxtx;
  /// condition variable for tx processing thread
  pthread_cond_t        cond_rxtx;
  /// mutex for RXn-TXnp4 processing thread
  pthread_mutex_t       mutex_rxtx;
  /// scheduling parameters for RXn-TXnp4 thread
  struct                sched_param sched_param_rxtx;
  /// 
  int                   sub_frame_start;
  ///
  int                   sub_frame_step;
  ///
  unsigned long long    gotIQs;

} UE_rxtx_proc_NB_IoT_t;

/// Context data structure for eNB subframe processing
typedef struct {
  /// Last RX timestamp
  openair0_timestamp      timestamp_rx;
  /// pthread attributes for main UE thread
  pthread_attr_t          attr_ue;
  /// scheduling parameters for main UE thread
  struct                  sched_param sched_param_ue;
  /// pthread descriptor main UE thread
  pthread_t               pthread_ue;
  /// \brief Instance count for synch thread.
  /// \internal This variable is protected by \ref mutex_synch.
  int                     instance_cnt_synch;
  /// pthread attributes for synch processing thread
  pthread_attr_t          attr_synch;
  /// scheduling parameters for synch thread
  struct                  sched_param sched_param_synch;
  /// pthread descriptor synch thread
  pthread_t               pthread_synch;
  /// condition variable for UE synch thread;
  pthread_cond_t          cond_synch;
  /// mutex for UE synch thread
  pthread_mutex_t         mutex_synch;
  /// set of scheduling variables RXn-TXnp4 threads
  UE_rxtx_proc_NB_IoT_t   proc_rxtx[2];

} UE_proc_NB_IoT_t;



/// Top-level PHY Data Structure for eNB
typedef struct PHY_VARS_eNB_NB_IoT_s {
  /// Module ID indicator for this instance
  module_id_t                   Mod_id;
  uint8_t                       configured;
  eNB_proc_NB_IoT_t             proc;
  int                           num_RU;
  RU_t                          *RU_list[MAX_NUM_RU_PER_eNB];
  /// Ethernet parameters for northbound midhaul interface (L1 to Mac)
  eth_params_t         eth_params_n;
  /// Ethernet parameters for fronthaul interface (upper L1 to Radio head)
  eth_params_t         eth_params;
  int                           single_thread_flag;
  openair0_rf_map               rf_map;
  int                           abstraction_flag;
  openair0_timestamp            ts_offset;
  // indicator for synchronization state of eNB
  int                           in_synch;
  // indicator for master/slave (RRU)
  int                           is_slave;
  // indicator for precoding function (eNB,3GPP_eNB_BBU)
  int                           do_precoding;
  IF_Module_NB_IoT_t            *if_inst_NB_IoT;
  UL_IND_NB_IoT_t               UL_INFO_NB_IoT;
  pthread_mutex_t               UL_INFO_mutex;
  void                          (*do_prach)(struct PHY_VARS_eNB_NB_IoT_s *eNB,int frame,int subframe);
  void                          (*fep)(struct PHY_VARS_eNB_NB_IoT_s *eNB,eNB_rxtx_proc_NB_IoT_t *proc);
  int                           (*td)(struct PHY_VARS_eNB_NB_IoT_s *eNB,int UE_id,int harq_pid,int llr8_flag);
  int                           (*te)(struct PHY_VARS_eNB_NB_IoT_s *,uint8_t *,uint8_t,NB_IoT_eNB_DLSCH_t *,int,uint8_t,time_stats_t *,time_stats_t *,time_stats_t *);
  void                          (*proc_uespec_rx)(struct PHY_VARS_eNB_NB_IoT_s *eNB,eNB_rxtx_proc_NB_IoT_t *proc,const relaying_type_t_NB_IoT r_type);
  void                          (*proc_tx)(struct PHY_VARS_eNB_NB_IoT_s *eNB,eNB_rxtx_proc_NB_IoT_t *proc,relaying_type_t_NB_IoT r_type,PHY_VARS_RN_NB_IoT *rn);
  void                          (*tx_fh)(struct PHY_VARS_eNB_NB_IoT_s *eNB,eNB_rxtx_proc_NB_IoT_t *proc);
  void                          (*rx_fh)(struct PHY_VARS_eNB_NB_IoT_s *eNB,int *frame, int *subframe);
  int                           (*start_rf)(struct PHY_VARS_eNB_NB_IoT_s *eNB);
  int                           (*start_if)(struct PHY_VARS_eNB_NB_IoT_s *eNB);
  void                          (*fh_asynch)(struct PHY_VARS_eNB_NB_IoT_s *eNB,int *frame, int *subframe);
  uint8_t                       local_flag;
  uint32_t                      rx_total_gain_dB;
  NB_IoT_DL_FRAME_PARMS         frame_parms;
  PHY_MEASUREMENTS_eNB_NB_IoT   measurements[NUMBER_OF_eNB_SECTORS_MAX_NB_IoT]; /// Measurement variables
  NB_IoT_eNB_COMMON             common_vars;
  NB_IoT_eNB_SRS                srs_vars[NUMBER_OF_UE_MAX_NB_IoT];
  NB_IoT_eNB_PBCH               pbch;
  NB_IoT_eNB_PUSCH              *pusch_vars[NUMBER_OF_UE_MAX_NB_IoT];
  NB_IoT_eNB_PRACH              prach_vars;
  //LTE_eNB_DLSCH_t             *dlsch[NUMBER_OF_UE_MAX_NB_IoT][2];             // Nusers times two spatial streams
  NB_IoT_eNB_NULSCH_t            *ulsch[NUMBER_OF_UE_MAX_NB_IoT+1];              // Nusers + number of RA (the ulsch[0] contains RAR)
  //LTE_eNB_DLSCH_t             *dlsch_SI,*dlsch_ra;
  //LTE_eNB_DLSCH_t             *dlsch_MCH;
  NB_IoT_eNB_UE_stats           UE_stats[NUMBER_OF_UE_MAX_NB_IoT];
  //LTE_eNB_UE_stats            *UE_stats_ptr[NUMBER_OF_UE_MAX_NB_IoT];
  /// cell-specific reference symbols
  uint32_t                      lte_gold_table_NB_IoT[20][2][14];
  /// UE-specific reference symbols (p=5), TM 7
  uint32_t                      lte_gold_uespec_port5_table[NUMBER_OF_UE_MAX_NB_IoT][20][38];
  /// UE-specific reference symbols (p=7...14), TM 8/9/10
  uint32_t                      lte_gold_uespec_table[2][20][2][21];
  /// mbsfn reference symbols
  uint32_t                      lte_gold_mbsfn_table[10][3][42];
  ///
  uint32_t                      X_u[64][839];
  ///
  uint8_t                       pbch_pdu[4];                                      //PBCH_PDU_SIZE
  ///
  char                          eNB_generate_rar;
  /// Indicator set to 0 after first SR
  uint8_t                       first_sr[NUMBER_OF_UE_MAX_NB_IoT];

  uint32_t                      max_peak_val;
  ///
  int                           max_eNB_id, max_sync_pos;
  ///
  int                           N_TA_offset;                        ///timing offset used in TDD
  /// \brief sinr for all subcarriers of the current link (used only for abstraction).
  /// first index: ? [0..N_RB_DL*12[
  double                        *sinr_dB;
  /// N0 (used for abstraction)
  double                        N0;
  ///
  unsigned char                 first_run_timing_advance[NUMBER_OF_UE_MAX_NB_IoT];
  unsigned char                 first_run_I0_measurements;

  unsigned char                 cooperation_flag;                   // for cooperative communication

  unsigned char                 is_secondary_eNB;                   // primary by default
  unsigned char                 is_init_sync;                       /// Flag to tell if initial synchronization is performed. This affects how often the secondary eNB will listen to the PSS from the primary system.
  unsigned char                 has_valid_precoder;                 /// Flag to tell if secondary eNB has channel estimates to create NULL-beams from, and this B/F vector is created.
  unsigned char                 PeNB_id;                            /// id of Primary eNB
  int                           rx_offset;                          /// Timing offset (used if is_secondary_eNB)

  /// hold the precoder for NULL beam to the primary user
  int                           **dl_precoder_SeNB[3];
  ///
  char                          log2_maxp;                          /// holds the maximum channel/precoder coefficient
  /// if ==0 enables phy only test mode
  int                           mac_enabled;
  /// For emulation only (used by UE abstraction to retrieve DCI)
  uint8_t                       num_common_dci[2];                  // num_dci in even/odd subframes
  ///
  uint8_t                       num_ue_spec_dci[2];                 // num_dci in even/odd subframes
  ///
  DCI_ALLOC_NB_IoT_t            dci_alloc[2][NUM_DCI_MAX_NB_IoT];   // dci_alloc from even/odd subframes
  /////////////
  // PDSCH Variables
  PDSCH_CONFIG_DEDICATED_NB_IoT             pdsch_config_dedicated[NUMBER_OF_UE_MAX_NB_IoT];
  // PUSCH Variables
  PUSCH_CONFIG_DEDICATED_NB_IoT             pusch_config_dedicated[NUMBER_OF_UE_MAX_NB_IoT];
  // PUCCH variables
  PUCCH_CONFIG_DEDICATED_NB_IoT             pucch_config_dedicated[NUMBER_OF_UE_MAX_NB_IoT];
  // UL-POWER-Control
  UL_POWER_CONTROL_DEDICATED_NB_IoT         ul_power_control_dedicated[NUMBER_OF_UE_MAX_NB_IoT];
  // TPC
  TPC_PDCCH_CONFIG_NB_IoT                   tpc_pdcch_config_pucch[NUMBER_OF_UE_MAX_NB_IoT];
  ///
  TPC_PDCCH_CONFIG_NB_IoT                   tpc_pdcch_config_pusch[NUMBER_OF_UE_MAX_NB_IoT];
  // CQI reporting
  CQI_REPORT_CONFIG_NB_IoT                  cqi_report_config[NUMBER_OF_UE_MAX_NB_IoT];
  // SRS Variables
  SOUNDINGRS_UL_CONFIG_DEDICATED_NB_IoT     soundingrs_ul_config_dedicated[NUMBER_OF_UE_MAX_NB_IoT];
  ///
  uint8_t                                   ncs_cell[20][7];
  // Scheduling Request Config
  SCHEDULING_REQUEST_CONFIG_NB_IoT          scheduling_request_config[NUMBER_OF_UE_MAX_NB_IoT];
  // Transmission mode per UE
  uint8_t                                   transmission_mode[NUMBER_OF_UE_MAX_NB_IoT];
  /// cba_last successful reception for each group, used for collision detection
  uint8_t                                   cba_last_reception[4];
  // Pointers for active physicalConfigDedicated to be applied in current subframe
  struct                                    PhysicalConfigDedicated                         *physicalConfigDedicated[NUMBER_OF_UE_MAX_NB_IoT];
  //Pointers for actve physicalConfigDedicated for NB-IoT to be applied in current subframe
  struct                                    PhysicalConfigDedicated_NB_r13                  *phy_config_dedicated_NB_IoT[NUMBER_OF_UE_MAX_NB_IoT];
  ///
  uint32_t                                  rb_mask_ul[4];
  /// Information regarding TM5
  MU_MIMO_mode_NB_IoT                       mu_mimo_mode[NUMBER_OF_UE_MAX_NB_IoT];
  /// target_ue_dl_mcs : only for debug purposes
  uint32_t                                  target_ue_dl_mcs;
  /// target_ue_ul_mcs : only for debug purposes
  uint32_t                                  target_ue_ul_mcs;
  /// target_ue_dl_rballoc : only for debug purposes
  uint32_t                                  ue_dl_rb_alloc;
  /// target ul PRBs : only for debug
  uint32_t                                  ue_ul_nb_rb;
  ///check for Total Transmissions
  uint32_t                                  check_for_total_transmissions;
  ///check for MU-MIMO Transmissions
  uint32_t                                  check_for_MUMIMO_transmissions;
  ///check for SU-MIMO Transmissions
  uint32_t                                  check_for_SUMIMO_transmissions;
  ///check for FULL MU-MIMO Transmissions
  uint32_t                                  FULL_MUMIMO_transmissions;
  /// Counter for total bitrate, bits and throughput in downlink
  uint32_t                                  total_dlsch_bitrate;
  ///
  uint32_t                                  total_transmitted_bits;
  ///
  uint32_t                                  total_system_throughput;
  ///
  int                                       hw_timing_advance;
  ///
  time_stats_t                       phy_proc;
  time_stats_t                       phy_proc_tx;
  time_stats_t                       phy_proc_rx;
  time_stats_t                       rx_prach;

  time_stats_t                       ofdm_mod_stats;
  time_stats_t                       dlsch_encoding_stats;
  time_stats_t                       dlsch_modulation_stats;
  time_stats_t                       dlsch_scrambling_stats;
  time_stats_t                       dlsch_rate_matching_stats;
  time_stats_t                       dlsch_turbo_encoding_stats;
  time_stats_t                       dlsch_interleaving_stats;

  time_stats_t                       ofdm_demod_stats;
  time_stats_t                       rx_dft_stats;
  time_stats_t                       ulsch_channel_estimation_stats;
  time_stats_t                       ulsch_freq_offset_estimation_stats;
  time_stats_t                       ulsch_decoding_stats;
  time_stats_t                       ulsch_demodulation_stats;
  time_stats_t                       ulsch_rate_unmatching_stats;
  time_stats_t                       ulsch_turbo_decoding_stats;
  time_stats_t                       ulsch_deinterleaving_stats;
  time_stats_t                       ulsch_demultiplexing_stats;
  time_stats_t                       ulsch_llr_stats;
  time_stats_t                       ulsch_tc_init_stats;
  time_stats_t                       ulsch_tc_alpha_stats;
  time_stats_t                       ulsch_tc_beta_stats;
  time_stats_t                       ulsch_tc_gamma_stats;
  time_stats_t                       ulsch_tc_ext_stats;
  time_stats_t                       ulsch_tc_intl1_stats;
  time_stats_t                       ulsch_tc_intl2_stats;

  #ifdef LOCALIZATION
  /// time state for localization
  time_stats_t                       localization_stats;
  #endif

  int32_t                                   pucch1_stats_cnt[NUMBER_OF_UE_MAX_NB_IoT][10];
  int32_t                                   pucch1_stats[NUMBER_OF_UE_MAX_NB_IoT][10*1024];
  int32_t                                   pucch1_stats_thres[NUMBER_OF_UE_MAX_NB_IoT][10*1024];
  int32_t                                   pucch1ab_stats_cnt[NUMBER_OF_UE_MAX_NB_IoT][10];
  int32_t                                   pucch1ab_stats[NUMBER_OF_UE_MAX_NB_IoT][2*10*1024];
  int32_t                                   pusch_stats_rb[NUMBER_OF_UE_MAX_NB_IoT][10240];
  int32_t                                   pusch_stats_round[NUMBER_OF_UE_MAX_NB_IoT][10240];
  int32_t                                   pusch_stats_mcs[NUMBER_OF_UE_MAX_NB_IoT][10240];
  int32_t                                   pusch_stats_bsr[NUMBER_OF_UE_MAX_NB_IoT][10240];
  int32_t                                   pusch_stats_BO[NUMBER_OF_UE_MAX_NB_IoT][10240];

  /// RF and Interface devices per CC
  openair0_device                           rfdevice;
  openair0_device                           ifdevice;
  /// Pointer for ifdevice buffer struct
  if_buffer_t                               ifbuffer;

  //------------------------
  // NB-IoT
  //------------------------

  /*
   * NUMBER_OF_UE_MAX_NB_IoT maybe in the future should be dynamic because could be very large and the memory may explode
   * (is almost the indication of the number of UE context that we are storing at PHY layer)
   *
   * reasoning: the following data structure (ndlsch, nulsch ecc..) are used to store the context that should be transmitted in at least n+4 subframe later
   * (the minimum interval between NPUSCH and the ACK for this)
   * the problem is that in NB_IoT the ACK for the UPLINK is contained in the DCI through the NDI field (if this value change from the previous one then it means ACK)
   * but may we could schedule this DCI long time later so may lots of contents shuld be stored (there is no concept of phich channel in NB-IoT)
   * For the DL transmission the UE send a proper ACK/NACK message
   *
   * *the HARQ process should be killed when the NDI change
   *
   * *In the Structure for nulsch we should also store the information related to the subframe (because each time we should read it and understand what should be done
   * in that subframe)
   *
   */


  /*
   * TIMING
   * the entire transmission and scheduling are done for the "subframe" concept but the subframe = proc->subframe_tx (that in reality is the subframe_rx +4)
   * (see USER/lte-enb/wakeup_rxtx )
   *
   * Related to FAPI:
   * DCI and  DL_CONFIG.request (also more that 1) and MAC_PDU are transmitted in the same subframe (our assumption) so will be all contained in the schedule_response getting from the scheduler
   * DCI0 and UL_CONFIG.request are transmitted in the same subframe (our assumption) so contained in the schedule_response
   *
   */

  //TODO: check what should be NUMBER_OF_UE_MAX_NB_IoT value
  NB_IoT_eNB_NPBCH_t        *npbch;
  NB_IoT_eNB_NPDCCH_t       *npdcch[NUMBER_OF_UE_MAX_NB_IoT];
  NB_IoT_eNB_NDLSCH_t       *ndlsch[NUMBER_OF_UE_MAX_NB_IoT][2];
  NB_IoT_eNB_NULSCH_t       *nulsch[NUMBER_OF_UE_MAX_NB_IoT+1]; //nulsch[0] contains the RAR
  NB_IoT_eNB_NDLSCH_t       *ndlsch_SI,*ndlsch_ra, *ndlsch_SIB1;

  NB_IoT_DL_FRAME_PARMS     frame_parms_NB_IoT;
  // DCI for at most 2 DCI pdus
  DCI_PDU_NB_IoT            *DCI_pdu;



} PHY_VARS_eNB_NB_IoT;

//#define debug_msg if (((mac_xface->frame%100) == 0) || (mac_xface->frame < 50)) msg

/// Top-level PHY Data Structure for UE
typedef struct {
  /// \brief Module ID indicator for this instance
  uint8_t                       Mod_id;
  /// \brief Mapping of CC_id antennas to cards
  openair0_rf_map               rf_map;
  //uint8_t local_flag;
  /// \brief Indicator of current run mode of UE (normal_txrx, rx_calib_ue, no_L2_connect, debug_prach)
  runmode_NB_IoT_t              mode;
  /// \brief Indicator that UE should perform band scanning
  int                           UE_scan;
  /// \brief Indicator that UE should perform coarse scanning around carrier
  int                           UE_scan_carrier;
  /// \brief Indicator that UE is synchronized to an eNB
  int                           is_synchronized;
  /// Data structure for UE process scheduling
  UE_proc_NB_IoT_t              proc;
  /// Flag to indicate the UE shouldn't do timing correction at all
  int                           no_timing_correction;
  /// \brief Total gain of the TX chain (16-bit baseband I/Q to antenna)
  uint32_t                      tx_total_gain_dB;
  /// \brief Total gain of the RX chain (antenna to baseband I/Q) This is a function of rx_gain_mode (and the corresponding gain) and the rx_gain of the card.
  uint32_t                      rx_total_gain_dB;
  /// \brief Total gains with maximum RF gain stage (ExpressMIMO2/Lime)
  uint32_t                      rx_gain_max[4];
  /// \brief Total gains with medium RF gain stage (ExpressMIMO2/Lime)
  uint32_t                      rx_gain_med[4];
  /// \brief Total gains with bypassed RF gain stage (ExpressMIMO2/Lime)
  uint32_t                      rx_gain_byp[4];
  /// \brief Current transmit power
  int16_t                       tx_power_dBm[10];
  /// \brief Total number of REs in current transmission
  int                           tx_total_RE[10];
  /// \brief Maximum transmit power
  int8_t                        tx_power_max_dBm;
  /// \brief Number of eNB seen by UE
  uint8_t                       n_connected_eNB;
  /// \brief indicator that Handover procedure has been initiated
  uint8_t                       ho_initiated;
  /// \brief indicator that Handover procedure has been triggered
  uint8_t                       ho_triggered;
  /// \brief Measurement variables.
  PHY_MEASUREMENTS_NB_IoT       measurements;
  NB_IoT_DL_FRAME_PARMS         frame_parms;
  /// \brief Frame parame before ho used to recover if ho fails.
  NB_IoT_DL_FRAME_PARMS         frame_parms_before_ho;
  NB_IoT_UE_COMMON              common_vars;

  NB_IoT_UE_PDSCH     *pdsch_vars[2][NUMBER_OF_CONNECTED_eNB_MAX+1]; // two RxTx Threads
  NB_IoT_UE_DLSCH_t   *dlsch[2][NUMBER_OF_CONNECTED_eNB_MAX][2]; // two RxTx Threads
  //Paging parameters
  uint32_t                        IMSImod1024;
  uint32_t                        PF;
  uint32_t                        PO;
  // For abstraction-purposes only
  uint8_t                         sr[10];
  uint8_t                         pucch_sel[10];
  uint8_t                         pucch_payload[22];
  //UE_MODE_t                     UE_mode[NUMBER_OF_CONNECTED_eNB_MAX];
  //cell-specific reference symbols
  uint32_t                        lte_gold_table[7][20][2][14];
  //UE-specific reference symbols (p=5), TM 7
  uint32_t                        lte_gold_uespec_port5_table[20][38];
  //ue-specific reference symbols
  uint32_t                        lte_gold_uespec_table[2][20][2][21];
  //mbsfn reference symbols
  uint32_t                        lte_gold_mbsfn_table[10][3][42];
  ///
  uint32_t                        X_u[64][839];
  /// 
  uint32_t                        high_speed_flag;
  uint32_t                        perfect_ce;
  int16_t                         ch_est_alpha;
  int                             generate_ul_signal[NUMBER_OF_CONNECTED_eNB_MAX];
  ///
  UE_SCAN_INFO_NB_IoT_t           scan_info[NB_BANDS_MAX_NB_IoT];
  ///
  char                            ulsch_no_allocation_counter[NUMBER_OF_CONNECTED_eNB_MAX];

/*

  unsigned char ulsch_Msg3_active[NUMBER_OF_CONNECTED_eNB_MAX];
  uint32_t  ulsch_Msg3_frame[NUMBER_OF_CONNECTED_eNB_MAX];
  unsigned char ulsch_Msg3_subframe[NUMBER_OF_CONNECTED_eNB_MAX];
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
  int dlsch_p_received[NUMBER_OF_CONNECTED_eNB_MAX];
  int dlsch_p_errors[NUMBER_OF_CONNECTED_eNB_MAX];
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
  uint8_t               decode_SIB;
  uint8_t               decode_MIB;
  int              rx_offset; /// Timing offset
  int              rx_offset_diff; /// Timing adjustment for ofdm symbol0 on HW USRP
  int              timing_advance; ///timing advance signalled from eNB
  int              hw_timing_advance;
  int              N_TA_offset; ///timing offset used in TDD
  /// Flag to tell if UE is secondary user (cognitive mode)
  unsigned char    is_secondary_ue;
  /// Flag to tell if secondary eNB has channel estimates to create NULL-beams from.
  unsigned char    has_valid_precoder;
  /// hold the precoder for NULL beam to the primary eNB
  int              **ul_precoder_S_UE;
  /// holds the maximum channel/precoder coefficient
  char             log2_maxp;
*/
  /// if ==0 enables phy only test mode
  int              mac_enabled;
  /// Flag to initialize averaging of PHY measurements
  int              init_averaging;
  /// \brief sinr for all subcarriers of the current link (used only for abstraction).
  /// - first index: ? [0..12*N_RB_DL[
  double           *sinr_dB;
  /// \brief sinr for all subcarriers of first symbol for the CQI Calculation.
  /// - first index: ? [0..12*N_RB_DL[
  double           *sinr_CQI_dB;
  /// sinr_effective used for CQI calulcation
  double           sinr_eff;
  /// N0 (used for abstraction)
  double           N0;
/*
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
  time_stats_t phy_proc_rx[2];

  uint32_t use_ia_receiver;

  time_stats_t ofdm_mod_stats;
  time_stats_t ulsch_encoding_stats;
  time_stats_t ulsch_modulation_stats;
  time_stats_t ulsch_segmentation_stats;
  time_stats_t ulsch_rate_matching_stats;
  time_stats_t ulsch_turbo_encoding_stats;
  time_stats_t ulsch_interleaving_stats;
  time_stats_t ulsch_multiplexing_stats;

  time_stats_t generic_stat;
  time_stats_t pdsch_procedures_stat;
  time_stats_t dlsch_procedures_stat;

  time_stats_t ofdm_demod_stats;
  time_stats_t dlsch_rx_pdcch_stats;
  time_stats_t rx_dft_stats;
  time_stats_t dlsch_channel_estimation_stats;
  time_stats_t dlsch_freq_offset_estimation_stats;
  time_stats_t dlsch_decoding_stats[2];
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

  /// RF and Interface devices per CC
  openair0_device rfdevice;
  time_stats_t dlsch_encoding_SIC_stats;
  time_stats_t dlsch_scrambling_SIC_stats;
  time_stats_t dlsch_modulation_SIC_stats;
  time_stats_t dlsch_llr_stripping_unit_SIC_stats;
  time_stats_t dlsch_unscrambling_SIC_stats;

#if ENABLE_RAL
  hash_table_t    *ral_thresholds_timed;
  SLIST_HEAD(ral_thresholds_gen_poll_s, ral_threshold_phy_t) ral_thresholds_gen_polled[RAL_LINK_PARAM_GEN_MAX];
  SLIST_HEAD(ral_thresholds_lte_poll_s, ral_threshold_phy_t) ral_thresholds_lte_polled[RAL_LINK_PARAM_LTE_MAX];
#endif
*/
} PHY_VARS_UE_NB_IoT;



#include "PHY/INIT/defs_NB_IoT.h"
#include "PHY/LTE_REFSIG/defs_NB_IoT.h"
#include "PHY/LTE_TRANSPORT/proto_NB_IoT.h"
#endif //  __PHY_DEFS__H__
