#ifndef LTE_SOFTMODEM_H
#define LTE_SOFTMODEM_H

#define _GNU_SOURCE
#include <execinfo.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/sched.h>
#include "rt_wrapper.h"
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/sysinfo.h>
#include "rt_wrapper.h"
#include "../../ARCH/COMMON/common_lib.h"
#undef MALLOC
#include "assertions.h"
#include "msc.h"
#include "PHY/types.h"
#include "PHY/defs.h"
#include "SIMULATION/ETH_TRANSPORT/proto.h"

#if defined(ENABLE_ITTI)
#if defined(ENABLE_USE_MME)
#include "s1ap_eNB.h"
#ifdef PDCP_USE_NETLINK
#include "SIMULATION/ETH_TRANSPORT/proto.h"
#endif
#endif
#endif

extern pthread_cond_t sync_cond;
extern pthread_mutex_t sync_mutex;
extern int sync_var;


extern uint32_t          downlink_frequency[MAX_NUM_CCs][4];
extern int32_t           uplink_frequency_offset[MAX_NUM_CCs][4];

extern int rx_input_level_dBm;
extern uint8_t exit_missed_slots;
extern uint64_t num_missed_slots; // counter for the number of missed slots

extern int oaisim_flag;
extern volatile int  oai_exit;

extern openair0_config_t openair0_cfg[MAX_CARDS];
extern pthread_cond_t sync_cond;
extern pthread_mutex_t sync_mutex;
extern int sync_var;
extern int transmission_mode;
extern double cpuf;

#if defined(ENABLE_ITTI)
extern volatile int             start_eNB;
extern volatile int             start_UE;
#endif

#include "threads_t.h"
extern threads_t threads;

extern void exit_fun(const char* s);
// In lte-enb.c
extern int setup_eNB_buffers(PHY_VARS_eNB **phy_vars_eNB, openair0_config_t *openair0_cfg);
extern void init_eNB(eNB_func_t *, eNB_timing_t *,int,eth_params_t *,int,int);
extern void stop_eNB(int);
extern void kill_eNB_proc(int inst);

// In lte-ue.c
extern int setup_ue_buffers(PHY_VARS_UE **phy_vars_ue, openair0_config_t *openair0_cfg);
extern void fill_ue_band_info(void);
extern void init_UE(int);
extern void reset_opp_meas(void);
extern void print_opp_meas(void);

extern void init_fep_thread(PHY_VARS_eNB *, pthread_attr_t *);
extern void init_td_thread(PHY_VARS_eNB *, pthread_attr_t *);
extern void init_te_thread(PHY_VARS_eNB *, pthread_attr_t *);

#endif
