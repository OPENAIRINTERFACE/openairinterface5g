#ifndef THREAD_COMMON_H
#define THREAD_COMMON_H

#include "PHY/defs_common.h"

extern THREAD_STRUCT thread_struct;

static inline void set_parallel_conf(char *parallel_conf) {
  mapping config[]= {
    FOREACH_PARALLEL(GENERATE_ENUMTXT)
    {NULL,-1}
  };
  thread_struct.parallel_conf = (PARALLEL_CONF_t)map_str_to_int(config, parallel_conf);
  if (thread_struct.parallel_conf == (unsigned int)-1) {
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
  if (thread_struct.worker_conf == (unsigned int)-1) {
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

#endif
