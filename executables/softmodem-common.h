#ifndef SOFTMODEM_COMMON_H
#define SOFTMODEM_COMMON_H

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
