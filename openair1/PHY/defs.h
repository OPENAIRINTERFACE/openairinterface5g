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
#ifndef __PHY_DEFS__H__
#define __PHY_DEFS__H__


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


#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <math.h>
#include "common_lib.h"
#include "msc.h"

#include "openair2/PHY_INTERFACE/IF_Module.h"

//#include <complex.h>
#include "assertions.h"
#ifdef MEX
# define msg mexPrintf
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
#define free_and_zero(PtR) do { \
      if (PtR) {           \
        free(PtR);         \
        PtR = NULL;        \
      }                    \
    } while (0)

#define RX_NB_TH_MAX 2
#define RX_NB_TH 2


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
#define MAX_NUM_RU_PER_eNB 64 

#include "PHY/LTE_TRANSPORT/defs.h"
#include <pthread.h>

#include "targets/ARCH/COMMON/common_lib.h"
#include "targets/COMMON/openairinterface5g_limits.h"

#if defined(EXMIMO) || defined(OAI_USRP)
//#define NUMBER_OF_eNB_MAX 1
//#define NUMBER_OF_UE_MAX 16

//#define NUMBER_OF_CONNECTED_eNB_MAX 3
#else
#ifdef LARGE_SCALE
//#define NUMBER_OF_eNB_MAX 2
//#define NUMBER_OF_UE_MAX 120
//#define NUMBER_OF_CONNECTED_eNB_MAX 1 // to save some memory
#else
//#define NUMBER_OF_eNB_MAX 3
//#define NUMBER_OF_UE_MAX 16
//#define NUMBER_OF_RU_MAX 64
//#define NUMBER_OF_CONNECTED_eNB_MAX 1
#endif
#endif
#define NUMBER_OF_SUBBANDS_MAX 13
#define NUMBER_OF_HARQ_PID_MAX 8

#define MAX_FRAME_NUMBER 0x400



#define NUMBER_OF_RN_MAX 3
typedef enum {no_relay=1,unicast_relay_type1,unicast_relay_type2, multicast_relay} relaying_type_t;



#define MCS_COUNT 28
#define MCS_TABLE_LENGTH_MAX 64


#define NUM_DCI_MAX 32

#define NUMBER_OF_eNB_SECTORS_MAX 3

#define NB_BANDS_MAX 8

#define MAX_BANDS_PER_RRU 4


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
  MBP_RRU_IF5      // Mobipass RRU
} node_function_t;

typedef enum {

  synch_to_ext_device=0,  // synch to RF or Ethernet device
  synch_to_other,          // synch to another source_(timer, other RU)
  synch_to_mobipass_standalone  // special case for mobipass in standalone mode
} node_timing_t;
#endif

typedef struct UE_SCAN_INFO_s {
  /// 10 best amplitudes (linear) for each pss signals
  int32_t amp[3][10];
  /// 10 frequency offsets (kHz) corresponding to best amplitudes, with respect do minimum DL frequency in the band
  int32_t freq_offset_Hz[3][10];
} UE_SCAN_INFO_t;

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
} eNB_rxtx_proc_t;

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
} te_params;

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
#ifdef Rel14
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
#ifdef Rel14
  /// frame to act upon for reception of prach
  int frame_prach_br;
#endif
  /// frame offset for slave RUs (to correct for frame asynchronism at startup)
  int frame_offset;
  /// \brief Instance count for FH processing thread.
  /// \internal This variable is protected by \ref mutex_FH.
  int instance_cnt_FH;
  /// \internal This variable is protected by \ref mutex_prach.
  int instance_cnt_prach;
#ifdef Rel14
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
  /// \internal This variable is protected by \ref mutex_fep
  int instance_cnt_feptx;
  /// pthread structure for RU FH processing thread
  pthread_t pthread_FH;
  /// pthread structure for RU prach processing thread
  pthread_t pthread_prach;
#ifdef Rel14
  /// pthread structure for RU prach processing thread BL/CE UEs
  pthread_t pthread_prach_br;
#endif
  /// pthread struct for RU synch thread
  pthread_t pthread_synch;
  /// pthread struct for RU RX FEP worker thread
  pthread_t pthread_fep;
  /// pthread struct for RU RX FEPTX worker thread
  pthread_t pthread_feptx;
  /// pthread structure for asychronous RX/TX processing thread
  pthread_t pthread_asynch_rxtx;
  /// flag to indicate first RX acquisition
  int first_rx;
  /// flag to indicate first TX transmission
  int first_tx;
  /// pthread attributes for RU FH processing thread
  pthread_attr_t attr_FH;
  /// pthread attributes for RU prach
  pthread_attr_t attr_prach;
#ifdef Rel14
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
  /// scheduling parameters for RU FH thread
  struct sched_param sched_param_FH;
  /// scheduling parameters for RU prach thread
  struct sched_param sched_param_prach;
#ifdef Rel14
  /// scheduling parameters for RU prach thread BL/CE UEs
  struct sched_param sched_param_prach_br;
#endif
  /// scheduling parameters for RU synch thread
  struct sched_param sched_param_synch;
  /// scheduling parameters for asynch_rxtx thread
  struct sched_param sched_param_asynch_rxtx;
  /// condition variable for RU FH thread
  pthread_cond_t cond_FH;
  /// condition variable for RU prach thread
  pthread_cond_t cond_prach;
#ifdef Rel14
  /// condition variable for RU prach thread BL/CE UEs
  pthread_cond_t cond_prach_br;
#endif
  /// condition variable for RU synch thread
  pthread_cond_t cond_synch;
  /// condition variable for asynch RX/TX thread
  pthread_cond_t cond_asynch_rxtx;
  /// condition varaible for RU RX FEP thread
  pthread_cond_t cond_fep;
  /// condition varaible for RU RX FEPTX thread
  pthread_cond_t cond_feptx;
  /// condition variable for eNB signal
  pthread_cond_t cond_eNBs;
  /// mutex for RU FH
  pthread_mutex_t mutex_FH;
  /// mutex for RU prach
  pthread_mutex_t mutex_prach;
#ifdef Rel14
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
  /// symbol mask for IF4p5 reception per subframe
  uint32_t symbol_mask[10];
  /// number of slave threads
  int                  num_slaves;
  /// array of pointers to slaves
  struct RU_proc_t_s           **slave_proc;
} RU_proc_t;

/// Context data structure for eNB subframe processing
typedef struct eNB_proc_t_s {
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
#ifdef Rel14
  /// subframe to act upon for reception of prach BL/CE UEs
  int subframe_prach_br;
#endif
  /// frame to act upon for reception
  int frame_rx;
  /// frame to act upon for transmission
  int frame_tx;
  /// frame to act upon for PRACH
  int frame_prach;
#ifdef Rel14
  /// frame to act upon for PRACH BL/CE UEs
  int frame_prach_br;
#endif
  /// \internal This variable is protected by \ref mutex_td.
  int instance_cnt_td;
  /// \internal This variable is protected by \ref mutex_te.
  int instance_cnt_te;
  /// \internal This variable is protected by \ref mutex_prach.
  int instance_cnt_prach;
#ifdef Rel14
  /// \internal This variable is protected by \ref mutex_prach for BL/CE UEs.
  int instance_cnt_prach_br;
#endif
  // instance count for over-the-air eNB synchronization
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
#ifdef Rel14
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
#ifdef Rel14
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
#ifdef Rel14
  /// pthread structure for PRACH thread BL/CE UEs
  pthread_t pthread_prach_br;
#endif
  /// condition variable for parallel turbo-decoder thread
  pthread_cond_t cond_td;
  /// condition variable for parallel turbo-encoder thread
  pthread_cond_t cond_te;
  /// condition variable for PRACH processing thread;
  pthread_cond_t cond_prach;
#ifdef Rel14
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
#ifdef Rel14
  /// mutex for PRACH thread for BL/CE UEs
  pthread_mutex_t mutex_prach_br;
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
  int RU_mask;
  /// mask for RUs serving eNB (PRACH)
  int RU_mask_prach;
#ifdef Rel14
  /// mask for RUs serving eNB (PRACH)
  int RU_mask_prach_br;
#endif
  /// parameters for turbo-decoding worker thread
  td_params tdp;
  /// parameters for turbo-encoding worker thread
  te_params tep;
  /// set of scheduling variables RXn-TXnp4 threads
  eNB_rxtx_proc_t proc_rxtx[2];
} eNB_proc_t;


/// Context data structure for RX/TX portion of subframe processing
typedef struct {
  /// index of the current UE RX/TX proc
  int                  proc_id;
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

  /// internal This variable is protected by ref mutex_fep_slot1.
  //int instance_cnt_slot0_dl_processing;
  int instance_cnt_slot1_dl_processing;
  /// pthread descriptor fep_slot1 thread
  //pthread_t pthread_slot0_dl_processing;
  pthread_t pthread_slot1_dl_processing;
  /// pthread attributes for fep_slot1 processing thread
 // pthread_attr_t attr_slot0_dl_processing;
  pthread_attr_t attr_slot1_dl_processing;
  /// condition variable for UE fep_slot1 thread;
  //pthread_cond_t cond_slot0_dl_processing;
  pthread_cond_t cond_slot1_dl_processing;
  /// mutex for UE synch thread
  //pthread_mutex_t mutex_slot0_dl_processing;
  pthread_mutex_t mutex_slot1_dl_processing;
  //
  uint8_t chan_est_pilot0_slot1_available;
  uint8_t chan_est_slot1_available;
  uint8_t llr_slot1_available;
  uint8_t dci_slot0_available;
  uint8_t first_symbol_available;
  //uint8_t channel_level;
  /// scheduling parameters for fep_slot1 thread
  struct sched_param sched_param_fep_slot1;

  int sub_frame_start;
  int sub_frame_step;
  unsigned long long gotIQs;
} UE_rxtx_proc_t;

/// Context data structure for eNB subframe processing
typedef struct {
  /// Component Carrier index
  uint8_t              CC_id;
  /// Last RX timestamp
  openair0_timestamp timestamp_rx;
  /// pthread attributes for main UE thread
  pthread_attr_t attr_ue;
  /// scheduling parameters for main UE thread
  struct sched_param sched_param_ue;
  /// pthread descriptor main UE thread
  pthread_t pthread_ue;
  /// \brief Instance count for synch thread.
  /// \internal This variable is protected by \ref mutex_synch.
  int instance_cnt_synch;
  /// pthread attributes for synch processing thread
  pthread_attr_t attr_synch;
  /// scheduling parameters for synch thread
  struct sched_param sched_param_synch;
  /// pthread descriptor synch thread
  pthread_t pthread_synch;
  /// condition variable for UE synch thread;
  pthread_cond_t cond_synch;
  /// mutex for UE synch thread
  pthread_mutex_t mutex_synch;
  /// instance count for eNBs
  int instance_cnt_eNBs;
  /// set of scheduling variables RXn-TXnp4 threads
  UE_rxtx_proc_t proc_rxtx[RX_NB_TH];
} UE_proc_t;

typedef enum {
  LOCAL_RF        =0,
  REMOTE_IF5      =1,
  REMOTE_MBP_IF5  =2,
  REMOTE_IF4p5    =3,
  REMOTE_IF1pp    =4,
  MAX_RU_IF_TYPES =5
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
  LTE_DL_FRAME_PARMS frame_parms;
  ///timing offset used in TDD
  int              N_TA_offset; 
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
#ifdef Rel14
  /// function pointer to wakeup routine in lte-enb.
  void (*wakeup_prach_eNB_br)(struct PHY_VARS_eNB_s *eNB,struct RU_t_s *ru,int frame,int subframe);
#endif
  /// function pointer to eNB entry routine
  void (*eNB_top)(struct PHY_VARS_eNB_s *eNB, int frame_rx, int subframe_rx, char *string);
  /// Timing statistics
  time_stats_t ofdm_demod_stats;
  /// Timing statistics (TX)
  time_stats_t ofdm_mod_stats;
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


typedef struct {
  //unsigned int   rx_power[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX];     //! estimated received signal power (linear)
  //unsigned short rx_power_dB[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX];  //! estimated received signal power (dB)
  //unsigned short rx_avg_power_dB[NUMBER_OF_CONNECTED_eNB_MAX];              //! estimated avg received signal power (dB)

  // RRC measurements
  uint32_t rssi;
  int n_adj_cells;
  unsigned int adj_cell_id[6];
  uint32_t rsrq[7];
  uint32_t rsrp[7];
  float rsrp_filtered[7]; // after layer 3 filtering
  float rsrq_filtered[7];
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
  int            rx_spatial_power[NUMBER_OF_CONNECTED_eNB_MAX][2][2];
  //! estimated received spatial signal power (dB)
  unsigned short rx_spatial_power_dB[NUMBER_OF_CONNECTED_eNB_MAX][2][2];

  /// estimated received signal power (sum over all TX antennas)
  //int            wideband_cqi[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX];
  int            rx_power[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX];
  /// estimated received signal power (sum over all TX antennas)
  //int            wideband_cqi_dB[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX];
  unsigned short rx_power_dB[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX];

  /// estimated received signal power (sum over all TX/RX antennas)
  int            rx_power_tot[NUMBER_OF_CONNECTED_eNB_MAX]; //NEW
  /// estimated received signal power (sum over all TX/RX antennas)
  unsigned short rx_power_tot_dB[NUMBER_OF_CONNECTED_eNB_MAX]; //NEW

  //! estimated received signal power (sum of all TX/RX antennas, time average)
  int            rx_power_avg[NUMBER_OF_CONNECTED_eNB_MAX];
  //! estimated received signal power (sum of all TX/RX antennas, time average, in dB)
  unsigned short rx_power_avg_dB[NUMBER_OF_CONNECTED_eNB_MAX];

  /// SINR (sum of all TX/RX antennas, in dB)
  int            wideband_cqi_tot[NUMBER_OF_CONNECTED_eNB_MAX];
  /// SINR (sum of all TX/RX antennas, time average, in dB)
  int            wideband_cqi_avg[NUMBER_OF_CONNECTED_eNB_MAX];

  //! estimated rssi (dBm)
  short          rx_rssi_dBm[NUMBER_OF_CONNECTED_eNB_MAX];
  //! estimated correlation (wideband linear) between spatial channels (computed in dlsch_demodulation)
  int            rx_correlation[NUMBER_OF_CONNECTED_eNB_MAX][2];
  //! estimated correlation (wideband dB) between spatial channels (computed in dlsch_demodulation)
  int            rx_correlation_dB[NUMBER_OF_CONNECTED_eNB_MAX][2];

  /// Wideband CQI (sum of all RX antennas, in dB, for precoded transmission modes (3,4,5,6), up to 4 spatial streams)
  int            precoded_cqi_dB[NUMBER_OF_CONNECTED_eNB_MAX+1][4];
  /// Subband CQI per RX antenna (= SINR)
  int            subband_cqi[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX][NUMBER_OF_SUBBANDS_MAX];
  /// Total Subband CQI  (= SINR)
  int            subband_cqi_tot[NUMBER_OF_CONNECTED_eNB_MAX][NUMBER_OF_SUBBANDS_MAX];
  /// Subband CQI in dB (= SINR dB)
  int            subband_cqi_dB[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX][NUMBER_OF_SUBBANDS_MAX];
  /// Total Subband CQI
  int            subband_cqi_tot_dB[NUMBER_OF_CONNECTED_eNB_MAX][NUMBER_OF_SUBBANDS_MAX];
  /// Wideband PMI for each RX antenna
  int            wideband_pmi_re[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX];
  /// Wideband PMI for each RX antenna
  int            wideband_pmi_im[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX];
  ///Subband PMI for each RX antenna
  int            subband_pmi_re[NUMBER_OF_CONNECTED_eNB_MAX][NUMBER_OF_SUBBANDS_MAX][NB_ANTENNAS_RX];
  ///Subband PMI for each RX antenna
  int            subband_pmi_im[NUMBER_OF_CONNECTED_eNB_MAX][NUMBER_OF_SUBBANDS_MAX][NB_ANTENNAS_RX];
  /// chosen RX antennas (1=Rx antenna 1, 2=Rx antenna 2, 3=both Rx antennas)
  unsigned char           selected_rx_antennas[NUMBER_OF_CONNECTED_eNB_MAX][NUMBER_OF_SUBBANDS_MAX];
  /// Wideband Rank indication
  unsigned char  rank[NUMBER_OF_CONNECTED_eNB_MAX];
  /// Number of RX Antennas
  unsigned char  nb_antennas_rx;
  /// DLSCH error counter
  // short          dlsch_errors;

} PHY_MEASUREMENTS;

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
  eNB_proc_t           proc;
  int                  single_thread_flag;
  int                  abstraction_flag;
  int                  num_RU;
  RU_t                 *RU_list[MAX_NUM_RU_PER_eNB];
  /// Ethernet parameters for northbound midhaul interface
  eth_params_t         eth_params_n;
  /// Ethernet parameters for fronthaul interface
  eth_params_t         eth_params;
  int                  rx_total_gain_dB;
  int                  (*td)(struct PHY_VARS_eNB_s *eNB,int UE_id,int harq_pid,int llr8_flag);
  int                  (*te)(struct PHY_VARS_eNB_s *,uint8_t *,uint8_t,LTE_eNB_DLSCH_t *,int,uint8_t,time_stats_t *,time_stats_t *,time_stats_t *);
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
#ifdef Rel14
  /// NFAPI PRACH information BL/CE UEs
  nfapi_preamble_pdu_t preamble_list_br[MAX_NUM_RX_PRACH_PREAMBLES];
#endif
  Sched_Rsp_t          Sched_INFO;
  LTE_eNB_PDCCH        pdcch_vars[2];
  LTE_eNB_PHICH        phich_vars[2];
#ifdef Rel14
  LTE_eNB_EPDCCH       epdcch_vars[2];
  LTE_eNB_MPDCCH       mpdcch_vars[2];
  LTE_eNB_PRACH        prach_vars_br;
#endif
  LTE_eNB_COMMON       common_vars;
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

  /// cell-specific reference symbols
  uint32_t         lte_gold_table[20][2][14];

  /// UE-specific reference symbols (p=5), TM 7
  uint32_t         lte_gold_uespec_port5_table[NUMBER_OF_UE_MAX][20][38];

  /// UE-specific reference symbols (p=7...14), TM 8/9/10
  uint32_t         lte_gold_uespec_table[2][20][2][21];

  /// mbsfn reference symbols
  uint32_t         lte_gold_mbsfn_table[10][3][42];

  uint32_t X_u[64][839];
#ifdef Rel14
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
} PHY_VARS_eNB;

#define debug_msg if (((mac_xface->frame%100) == 0) || (mac_xface->frame < 50)) msg

/// Top-level PHY Data Structure for UE
typedef struct {
  /// \brief Module ID indicator for this instance
  uint8_t Mod_id;
  /// \brief Component carrier ID for this PHY instance
  uint8_t CC_id;
  /// \brief Mapping of CC_id antennas to cards
  openair0_rf_map      rf_map;
  //uint8_t local_flag;
  /// \brief Indicator of current run mode of UE (normal_txrx, rx_calib_ue, no_L2_connect, debug_prach)
  runmode_t mode;
  /// \brief Indicator that UE should perform band scanning
  int UE_scan;
  /// \brief Indicator that UE should perform coarse scanning around carrier
  int UE_scan_carrier;
  /// \brief Indicator that UE is synchronized to an eNB
  int is_synchronized;
  /// Data structure for UE process scheduling
  UE_proc_t proc;
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
  int16_t tx_power_dBm[10];
  /// \brief Total number of REs in current transmission
  int tx_total_RE[10];
  /// \brief Maximum transmit power
  int8_t tx_power_max_dBm;
  /// \brief Number of eNB seen by UE
  uint8_t n_connected_eNB;
  /// \brief indicator that Handover procedure has been initiated
  uint8_t ho_initiated;
  /// \brief indicator that Handover procedure has been triggered
  uint8_t ho_triggered;
  /// \brief Measurement variables.
  PHY_MEASUREMENTS measurements;
  LTE_DL_FRAME_PARMS  frame_parms;
  /// \brief Frame parame before ho used to recover if ho fails.
  LTE_DL_FRAME_PARMS  frame_parms_before_ho;
  LTE_UE_COMMON    common_vars;

  // point to the current rxTx thread index
  uint8_t current_thread_id[10];

  LTE_UE_PDSCH     *pdsch_vars[RX_NB_TH_MAX][NUMBER_OF_CONNECTED_eNB_MAX+1]; // two RxTx Threads
  LTE_UE_PDSCH_FLP *pdsch_vars_flp[NUMBER_OF_CONNECTED_eNB_MAX+1];
  LTE_UE_PDSCH     *pdsch_vars_SI[NUMBER_OF_CONNECTED_eNB_MAX+1];
  LTE_UE_PDSCH     *pdsch_vars_ra[NUMBER_OF_CONNECTED_eNB_MAX+1];
  LTE_UE_PDSCH     *pdsch_vars_p[NUMBER_OF_CONNECTED_eNB_MAX+1];
  LTE_UE_PDSCH     *pdsch_vars_MCH[NUMBER_OF_CONNECTED_eNB_MAX];
  LTE_UE_PBCH      *pbch_vars[NUMBER_OF_CONNECTED_eNB_MAX];
  LTE_UE_PDCCH     *pdcch_vars[RX_NB_TH_MAX][NUMBER_OF_CONNECTED_eNB_MAX];
  LTE_UE_PRACH     *prach_vars[NUMBER_OF_CONNECTED_eNB_MAX];
  LTE_UE_DLSCH_t   *dlsch[RX_NB_TH_MAX][NUMBER_OF_CONNECTED_eNB_MAX][2]; // two RxTx Threads
  LTE_UE_ULSCH_t   *ulsch[NUMBER_OF_CONNECTED_eNB_MAX];
  LTE_UE_DLSCH_t   *dlsch_SI[NUMBER_OF_CONNECTED_eNB_MAX];
  LTE_UE_DLSCH_t   *dlsch_ra[NUMBER_OF_CONNECTED_eNB_MAX];
  LTE_UE_DLSCH_t   *dlsch_p[NUMBER_OF_CONNECTED_eNB_MAX];
  LTE_UE_DLSCH_t   *dlsch_MCH[NUMBER_OF_CONNECTED_eNB_MAX];
  // This is for SIC in the UE, to store the reencoded data
  LTE_eNB_DLSCH_t  *dlsch_eNB[NUMBER_OF_CONNECTED_eNB_MAX];

  //Paging parameters
  uint32_t              IMSImod1024;
  uint32_t              PF;
  uint32_t              PO;

  // For abstraction-purposes only
  uint8_t               sr[10];
  uint8_t               pucch_sel[10];
  uint8_t               pucch_payload[22];

  UE_MODE_t        UE_mode[NUMBER_OF_CONNECTED_eNB_MAX];
  /// cell-specific reference symbols
  uint32_t lte_gold_table[7][20][2][14];

  /// UE-specific reference symbols (p=5), TM 7
  uint32_t lte_gold_uespec_port5_table[20][38];

  /// ue-specific reference symbols
  uint32_t lte_gold_uespec_table[2][20][2][21];

  /// mbsfn reference symbols
  uint32_t lte_gold_mbsfn_table[10][3][42];

  uint32_t X_u[64][839];

  uint32_t high_speed_flag;
  uint32_t perfect_ce;
  int16_t ch_est_alpha;
  int generate_ul_signal[NUMBER_OF_CONNECTED_eNB_MAX];

  UE_SCAN_INFO_t scan_info[NB_BANDS_MAX];

  char ulsch_no_allocation_counter[NUMBER_OF_CONNECTED_eNB_MAX];



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
  int              time_sync_cell;
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

  time_stats_t phy_proc[RX_NB_TH];
  time_stats_t phy_proc_tx;
  time_stats_t phy_proc_rx[RX_NB_TH];

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
  time_stats_t generic_stat_bis[RX_NB_TH][LTE_SLOTS_PER_SUBFRAME];
  time_stats_t ue_front_end_stat[RX_NB_TH];
  time_stats_t ue_front_end_per_slot_stat[RX_NB_TH][LTE_SLOTS_PER_SUBFRAME];
  time_stats_t pdcch_procedures_stat[RX_NB_TH];
  time_stats_t pdsch_procedures_stat[RX_NB_TH];
  time_stats_t pdsch_procedures_per_slot_stat[RX_NB_TH][LTE_SLOTS_PER_SUBFRAME];
  time_stats_t dlsch_procedures_stat[RX_NB_TH];

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
  time_stats_t dlsch_llr_stats_parallelization[RX_NB_TH][LTE_SLOTS_PER_SUBFRAME];
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
} PHY_VARS_UE;

/* this structure is used to pass both UE phy vars and
 * proc to the function UE_thread_rxn_txnp4
 */
struct rx_tx_thread_data {
  PHY_VARS_UE    *UE;
  UE_rxtx_proc_t *proc;
};

void exit_fun(const char* s);

#include "UTIL/LOG/log_extern.h"
extern pthread_cond_t sync_cond;
extern pthread_mutex_t sync_mutex;
extern int sync_var;


#define MODE_DECODE_NONE         0
#define MODE_DECODE_SSE          1
#define MODE_DECODE_C            2
#define MODE_DECODE_AVX2         3

#define DECODE_INITTD8_SSE_FPTRIDX   0
#define DECODE_INITTD16_SSE_FPTRIDX  1
#define DECODE_INITTD_AVX2_FPTRIDX   2
#define DECODE_TD8_SSE_FPTRIDX       3
#define DECODE_TD16_SSE_FPTRIDX      4
#define DECODE_TD_C_FPTRIDX          5
#define DECODE_TD16_AVX2_FPTRIDX     6
#define DECODE_FREETD8_FPTRIDX       7
#define DECODE_FREETD16_FPTRIDX      8
#define DECODE_FREETD_AVX2_FPTRIDX   9
#define ENCODE_SSE_FPTRIDX           10
#define ENCODE_C_FPTRIDX             11
#define ENCODE_INIT_SSE_FPTRIDX      12
#define DECODE_NUM_FPTR              13


typedef uint8_t(*decoder_if_t)(int16_t *y,
                               int16_t *y2,
    		               uint8_t *decoded_bytes,
    		               uint8_t *decoded_bytes2,
	   		       uint16_t n,
	   		       uint16_t f1,
	   		       uint16_t f2,
	   		       uint8_t max_iterations,
	   		       uint8_t crc_type,
	   		       uint8_t F,
	   		       time_stats_t *init_stats,
	   		       time_stats_t *alpha_stats,
	   		       time_stats_t *beta_stats,
	   		       time_stats_t *gamma_stats,
	   		       time_stats_t *ext_stats,
	   		       time_stats_t *intl1_stats,
                               time_stats_t *intl2_stats);

typedef uint8_t(*encoder_if_t)(uint8_t *input,
                               uint16_t input_length_bytes,
                               uint8_t *output,
                               uint8_t F,
                               uint16_t interleaver_f1,
                               uint16_t interleaver_f2);

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
#ifdef Rel14
  int emtc_prach_CElevel_enable[MAX_BANDS_PER_RRU][4];
  /// emtc_prach_FreqOffset for IF4p5 per CE Level
  int emtc_prach_FreqOffset[MAX_BANDS_PER_RRU][4];
  /// emtc_prach_ConfigIndex for IF4p5 per CE Level
  int emtc_prach_ConfigIndex[MAX_BANDS_PER_RRU][4];
#endif
} RRU_config_t;


static inline void wait_sync(char *thread_name) {

  printf( "waiting for sync (%s)\n",thread_name);
  pthread_mutex_lock( &sync_mutex );
  
  while (sync_var<0)
    pthread_cond_wait( &sync_cond, &sync_mutex );
  
  pthread_mutex_unlock(&sync_mutex);
  
  printf( "got sync (%s)\n", thread_name);

}

static inline int wait_on_condition(pthread_mutex_t *mutex,pthread_cond_t *cond,int *instance_cnt,char *name) {
  if (pthread_mutex_lock(mutex) != 0) {
    LOG_E( PHY, "[SCHED][eNB] error locking mutex for %s\n",name);
    exit_fun("nothing to add");
    return(-1);
  }

  while (*instance_cnt < 0) {
    // most of the time the thread is waiting here
    // proc->instance_cnt_rxtx is -1
    pthread_cond_wait(cond,mutex); // this unlocks mutex_rxtx while waiting and then locks it again
  }

  if (pthread_mutex_unlock(mutex) != 0) {
    LOG_E(PHY,"[SCHED][eNB] error unlocking mutex for %s\n",name);
    exit_fun("nothing to add");
    return(-1);
  }
  return(0);
}

static inline int wait_on_busy_condition(pthread_mutex_t *mutex,pthread_cond_t *cond,int *instance_cnt,char *name) {

  if (pthread_mutex_lock(mutex) != 0) {
    LOG_E( PHY, "[SCHED][eNB] error locking mutex for %s\n",name);
    exit_fun("nothing to add");
    return(-1);
  }

  while (*instance_cnt == 0) {
    // most of the time the thread will skip this
    // waits only if proc->instance_cnt_rxtx is 0
    pthread_cond_wait(cond,mutex); // this unlocks mutex_rxtx while waiting and then locks it again
  }

  if (pthread_mutex_unlock(mutex) != 0) {
    LOG_E(PHY,"[SCHED][eNB] error unlocking mutex for %s\n",name);
    exit_fun("nothing to add");
    return(-1);
  }
  return(0);
}

static inline int release_thread(pthread_mutex_t *mutex,int *instance_cnt,char *name) {

  if (pthread_mutex_lock(mutex) != 0) {
    LOG_E( PHY, "[SCHED][eNB] error locking mutex for %s\n",name);
    exit_fun("nothing to add");
    return(-1);
  }

  *instance_cnt=*instance_cnt-1;

  if (pthread_mutex_unlock(mutex) != 0) {
    LOG_E( PHY, "[SCHED][eNB] error unlocking mutex for %s\n",name);
    exit_fun("nothing to add");
    return(-1);
  }
  return(0);
}


#include "PHY/INIT/defs.h"
#include "PHY/LTE_REFSIG/defs.h"
#include "PHY/MODULATION/defs.h"
#include "PHY/LTE_TRANSPORT/proto.h"
#include "PHY/LTE_ESTIMATION/defs.h"

#include "SIMULATION/ETH_TRANSPORT/defs.h"
#endif //  __PHY_DEFS__H__
