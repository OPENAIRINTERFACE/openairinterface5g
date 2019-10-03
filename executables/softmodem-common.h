#ifndef SOFTMODEM_COMMON_H
#define SOFTMODEM_COMMON_H

//#include "threads_t.h"

THREAD_STRUCT thread_struct;

static inline void set_parallel_conf(char *parallel_conf) {
  mapping config[]= {
    FOREACH_PARALLEL(GENERATE_ENUMTXT)
    {NULL,-1}
  };
  thread_struct.parallel_conf = (PARALLEL_CONF_t)map_str_to_int(config, parallel_conf);
  if (thread_struct.parallel_conf == -1 ) {
    LOG_E(ENB_APP,"Impossible value: %s\n", parallel_conf);
    thread_struct.parallel_conf = PARALLEL_SINGLE_THREAD;
  }
  printf("[CONFIG] parallel_conf is set to %d\n", thread_struct.parallel_conf);
}

static inline void set_worker_conf(char *worker_conf) {
  mapping config[]={
    FOREACH_WORKER(GENERATE_ENUMTXT)
    {NULL, -1}
  };
  thread_struct.worker_conf = (WORKER_CONF_t)map_str_to_int(config, worker_conf);
  if (thread_struct.worker_conf == -1 ) {
    LOG_E(ENB_APP,"Impossible value: %s\n", worker_conf);
    thread_struct.worker_conf = WORKER_DISABLE ;
  }
  printf("[CONFIG] worker_conf is set to %d\n", thread_struct.worker_conf);
}

static inline PARALLEL_CONF_t get_thread_parallel_conf(void) {
  return thread_struct.parallel_conf;
}

static inline WORKER_CONF_t get_thread_worker_conf(void) {
  return thread_struct.worker_conf;
}

typedef struct {
  uint64_t       optmask;
  THREAD_STRUCT  thread_struct;
  char           rf_config_file[1024];
  int            phy_test;
  uint8_t        usim_test;
  int            emulate_rf;
  int            wait_for_sync; //eNodeB only
  int            single_thread_flag; //eNodeB only
  int            chain_offset;
  int            numerology;
  unsigned int   start_msc;
  uint32_t       clock_source;
  uint32_t       timing_source;
  int            hw_timing_advance;
  uint32_t       send_dmrs_sync;
} softmodem_params_t;





#endif
