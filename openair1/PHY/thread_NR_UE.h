#ifndef __thread_NR_UE__
#define __thread_NR_UE__
#include <pthread.h>
#include <targets/ARCH/COMMON/common_lib.h>
/// Context data structure for RX/TX portion of subframe processing
typedef struct {
  /// index of the current UE RX/TX thread
  int                  thread_id;
  /// Component Carrier index
  uint8_t              CC_id;
  /// timestamp transmitted to HW
  openair0_timestamp timestamp_tx;
  //#ifdef UE_NR_PHY_DEMO
  /// NR slot index within frame_tx [0 .. slots_per_frame - 1] to act upon for transmission
  int nr_slot_tx;
  /// NR slot index within frame_rx [0 .. slots_per_frame - 1] to act upon for transmission
  int nr_slot_rx;
  //#endif
  /// frame to act upon for transmission
  int frame_tx;
  /// frame to act upon for reception
  int frame_rx;
  int decoded_frame_rx;
  /// internal This variable is protected by ref mutex_fep_slot1.
  //int instance_cnt_slot0_dl_processing;
  int instance_cnt_slot1_dl_processing;
  /// pthread descriptor fep_slot1 thread
  //pthread_t pthread_slot0_dl_processing;
  pthread_t pthread_slot1_dl_processing;
  /// pthread attributes for fep_slot1 processing thread
  /// condition variable for UE fep_slot1 thread;
  //pthread_cond_t cond_slot0_dl_processing;
  pthread_cond_t cond_slot1_dl_processing;
  /// mutex for UE synch thread
  //pthread_mutex_t mutex_slot0_dl_processing;
  pthread_mutex_t mutex_slot1_dl_processing;
  //int instance_cnt_slot0_dl_processing;
  int instance_cnt_dlsch_td;
  /// pthread descriptor fep_slot1 thread
  //pthread_t pthread_slot0_dl_processing;
  pthread_t pthread_dlsch_td;
  /// pthread attributes for fep_slot1 processing thread
  /// condition variable for UE fep_slot1 thread;
  //pthread_cond_t cond_slot0_dl_processing;
  pthread_cond_t cond_dlsch_td;
  /// mutex for UE synch thread
  uint8_t chan_est_pilot0_slot1_available;
  uint8_t chan_est_slot1_available;
  uint8_t llr_slot1_available;
  uint8_t dci_slot0_available;
  uint8_t first_symbol_available;
  uint8_t decoder_thread_available;
  uint8_t decoder_main_available;
  uint8_t decoder_switch;
  int num_seg;
  uint8_t channel_level;
  int eNB_id;
  int harq_pid;
  int llr8_flag;
  /// scheduling parameters for fep_slot1 thread
  struct sched_param sched_param_fep_slot1;

  int sub_frame_start;
  int sub_frame_step;
  unsigned long long gotIQs;
  uint8_t decoder_thread_available1;
  int dci_err_cnt;
} UE_nr_rxtx_proc_t;

#endif
