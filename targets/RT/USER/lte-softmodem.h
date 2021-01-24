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
#include "threads_t.h"
#include "rt_wrapper.h"
#include "../../ARCH/COMMON/common_lib.h"
//#undef MALLOC
#include "assertions.h"
#include "msc.h"
#include "PHY/types.h"
#include "PHY/defs_eNB.h"
#include "PHY/defs_UE.h"
#include "flexran_agent.h"
#include "s1ap_eNB.h"
#include "SIMULATION/ETH_TRANSPORT/proto.h"
#include "proto_agent.h"
#include "executables/softmodem-common.h"



/***************************************************************************************************************************************/
/* command line options definitions, CMDLINE_XXXX_DESC macros are used to initialize paramdef_t arrays which are then used as argument
   when calling config_get or config_getlist functions                                                                                 */


/*------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters defining UE running mode                                              */
/*   optname                     helpstr                paramflags        XXXptr                      defXXXval           type      numelt  */
/*------------------------------------------------------------------------------------------------------------------------------------------*/
#define CMDLINE_UEMODEPARAMS_DESC {  \
    {"calib-ue-rx",              CONFIG_HLP_CALUER,     0,                iptr:&rx_input_level_dBm,   defintval:0,        TYPE_INT,   0},    \
    {"calib-ue-rx-med",          CONFIG_HLP_CALUERM,    0,                iptr:&rx_input_level_dBm,   defintval:0,        TYPE_INT,   0},    \
    {"calib-ue-rx-byp",          CONFIG_HLP_CALUERB,    0,                iptr:&rx_input_level_dBm,   defintval:0,        TYPE_INT,   0},    \
    {"debug-ue-prach",           CONFIG_HLP_DBGUEPR,    PARAMFLAG_BOOL,   uptr:NULL,                  defuintval:1,       TYPE_INT,   0},    \
    {"no-L2-connect",            CONFIG_HLP_NOL2CN,     PARAMFLAG_BOOL,   uptr:NULL,                  defuintval:1,       TYPE_INT,   0},    \
    {"calib-prach-tx",           CONFIG_HLP_CALPRACH,   PARAMFLAG_BOOL,   uptr:NULL,                  defuintval:1,       TYPE_INT,   0},    \
    {"loop-memory",              CONFIG_HLP_UELOOP,     0,                strptr:&loopfile,           defstrval:"iqs.in", TYPE_STRING,0},    \
    {"ue-dump-frame",            CONFIG_HLP_DUMPFRAME,  PARAMFLAG_BOOL,   iptr:&dumpframe,            defintval:0,        TYPE_INT,   0},    \
  }
#define CMDLINE_CALIBUERX_IDX                   0
#define CMDLINE_CALIBUERXMED_IDX                1
#define CMDLINE_CALIBUERXBYP_IDX                2
#define CMDLINE_DEBUGUEPRACH_IDX                3
#define CMDLINE_NOL2CONNECT_IDX                 4
#define CMDLINE_CALIBPRACHTX_IDX                5
#define CMDLINE_MEMLOOP_IDX                     6
#define CMDLINE_DUMPMEMORY_IDX                  7
/*------------------------------------------------------------------------------------------------------------------------------------------*/
/* help strings definition for command line options, used in CMDLINE_XXX_DESC macros and printed when -h option is used */


#define CONFIG_HLP_SIML1         "activate RF simulator instead of HW\n"
#define CONFIG_HLP_NUMUE         "number of UE instances\n"
#define CONFIG_HLP_UERXG         "set UE RX gain\n"
#define CONFIG_HLP_UERXGOFF      "external UE amplifier offset\n"
#define CONFIG_HLP_UETXG         "set UE TX gain\n"
#define CONFIG_HLP_UENANTR       "set UE number of rx antennas\n"
#define CONFIG_HLP_UENANTT       "set UE number of tx antennas\n"
#define CONFIG_HLP_UESCAN        "set UE to scan around carrier\n"
#define CONFIG_HLP_EMULIFACE     "Set the interface name for the multicast transport for emulation mode (e.g. eth0, lo, etc.)  \n"
#define CONFIG_HLP_PRB           "Set the PRB, valid values: 6, 25, 50, 100  \n"
#define CONFIG_HLP_DLSHIFT       "dynamic shift for LLR compuation for TM3/4 (default 0)\n"
#define CONFIG_HLP_USRP_ARGS     "set the arguments to identify USRP (same syntax as in UHD)\n"
#define CONFIG_HLP_DMAMAP        "sets flag for improved EXMIMO UE performance\n"
#define CONFIG_HLP_TDD           "Set hardware to TDD mode (default: FDD). Used only with -U (otherwise set in config file).\n"
#define CONFIG_HLP_TADV          "Set timing_advance\n"








/*-------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters specific to UE                                                                     */
/*   optname                     helpstr             paramflags          XXXptr                          defXXXval            type          numelt       */
/*-------------------------------------------------------------------------------------------------------------------------------------------------------*/

#define CMDLINE_UEPARAMS_DESC {  \
    {"U",                 CONFIG_HLP_NUMUE,       0,               u16ptr:&NB_UE_INST,                 defuintval:1,         TYPE_UINT16,   0},   \
    {"ue-rxgain",         CONFIG_HLP_UERXG,       0,               dblptr:&(rx_gain[0][0]),            defdblval:130,        TYPE_DOUBLE,   0},   \
    {"ue-rxgain-off",     CONFIG_HLP_UERXGOFF,    0,               dblptr:&rx_gain_off,                defdblval:0,          TYPE_DOUBLE,   0},   \
    {"ue-txgain",         CONFIG_HLP_UETXG,       0,               dblptr:&(tx_gain[0][0]),            defdblval:0,          TYPE_DOUBLE,   0},   \
    {"ue-nb-ant-rx",      CONFIG_HLP_UENANTR,     0,               u8ptr:&nb_antenna_rx,               defuintval:1,         TYPE_UINT8,    0},   \
    {"ue-nb-ant-tx",      CONFIG_HLP_UENANTT,     0,               u8ptr:&nb_antenna_tx,               defuintval:1,         TYPE_UINT8,    0},   \
    {"ue-scan-carrier",   CONFIG_HLP_UESCAN,      PARAMFLAG_BOOL,  iptr:&UE_scan_carrier,              defintval:0,          TYPE_INT,      0},   \
    {"ue-max-power",      NULL,                   0,               iptr:&(tx_max_power[0]),            defintval:23,         TYPE_INT,      0},   \
    {"emul-iface",        CONFIG_HLP_EMULIFACE,   0,               strptr:&emul_iface,                 defstrval:"lo",       TYPE_STRING, 100},   \
    {"L2-emul",           NULL,                   0,               u8ptr:&nfapi_mode,                  defuintval:3,         TYPE_UINT8,    0},   \
    {"num-ues",           NULL,                   0,               u16ptr:&(NB_UE_INST),               defuintval:1,         TYPE_UINT16,   0},   \
    {"nums_ue_thread",    NULL,                   0,               u16ptr:&(NB_THREAD_INST),           defuintval:1,         TYPE_UINT16,   0},   \
    {"r"  ,               CONFIG_HLP_PRB,         0,               u8ptr:&(frame_parms[0]->N_RB_DL),   defintval:25,         TYPE_UINT8,    0},   \
    {"dlsch-demod-shift", CONFIG_HLP_DLSHIFT,     0,               iptr:(int32_t *)&dlsch_demod_shift, defintval:0,          TYPE_INT,      0},   \
    {"usrp-args",         CONFIG_HLP_USRP_ARGS,   0,               strptr:(char **)&usrp_args,         defstrval:"type=b200",TYPE_STRING,   0},   \
    {"mmapped-dma",       CONFIG_HLP_DMAMAP,      PARAMFLAG_BOOL,  uptr:&mmapped_dma,                  defintval:0,          TYPE_INT,      0},   \
    {"T" ,                CONFIG_HLP_TDD,         PARAMFLAG_BOOL,  iptr:&tddflag,                      defintval:0,          TYPE_INT,      0},   \
    {"A",                 CONFIG_HLP_TADV,        0,               iptr:&(timingadv),                  defintval:0,          TYPE_INT,      0}    \
  }

/*-----------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters specific to UE threads                                   */
/*   optname                   helpstr     paramflags     XXXptr                       defXXXval        type          numelt   */
/*-----------------------------------------------------------------------------------------------------------------------------*/
#define CMDLINE_UETHREADSPARAMS_DESC {  \
    {"threadOneSubframe",       NULL,      0,     iptr:&(threads.one),                defintval:1,     TYPE_INT,       0},   \
    {"threadTwoSubframe",       NULL,      0,     iptr:&(threads.two),                defintval:1,     TYPE_INT,       0},   \
    {"threadThreeSubframe",     NULL,      0,     iptr:&(threads.three),              defintval:1,     TYPE_INT,       0},   \
    {"threadSlot1ProcOne",      NULL,      0,     iptr:&(threads.slot1_proc_one),     defintval:1,     TYPE_INT,       0},   \
    {"threadSlot1ProcTwo",      NULL,      0,     iptr:&(threads.slot1_proc_two),     defintval:1,     TYPE_INT,       0},   \
  }
//    {"threadIQ",                NULL,      0,     iptr:&(threads.iq),                 defintval:1,     TYPE_INT,       0},

#define DEFAULT_DLF 2680000000


uint64_t get_pdcp_optmask(void);
extern pthread_cond_t sync_cond;
extern pthread_mutex_t sync_mutex;
extern int sync_var;

extern uint64_t downlink_frequency[MAX_NUM_CCs][4];
extern int32_t  uplink_frequency_offset[MAX_NUM_CCs][4];

extern int rx_input_level_dBm;
extern uint64_t num_missed_slots; // counter for the number of missed slots

extern int oaisim_flag;
extern volatile int oai_exit;

extern openair0_config_t openair0_cfg[MAX_CARDS];
extern pthread_cond_t sync_cond;
extern pthread_mutex_t sync_mutex;
extern int sync_var;
extern int transmission_mode;
extern double cpuf;

extern int emulate_rf;
extern int numerology;
extern int usrp_tx_thread;

// In lte-enb.c
extern void stop_eNB(int);
extern void kill_eNB_proc(int inst);
extern void init_eNB(int single_thread_flag, int wait_for_sync);

// In lte-ru.c
extern void stop_ru(RU_t *ru);
extern void init_ru_vnf(void);
extern void init_RU_proc(RU_t *ru);
extern void stop_RU(int nb_ru);
extern void kill_RU_proc(RU_t *ru);
extern void set_function_spec_param(RU_t *ru);
extern void init_RU(RU_t **rup,int nb_RU,PHY_VARS_eNB ***eNBp,int nb_L1,int *nb_CC,char *rf_config_file, int send_dmrssync);

// In lte-ue.c
extern int setup_ue_buffers(PHY_VARS_UE **phy_vars_ue, openair0_config_t *openair0_cfg);
extern void fill_ue_band_info(void);

extern void init_UE(int nb_inst,
                    int eMBMS_active,
                    int uecap_xer_in,
                    int timing_correction,
                    int phy_test,
                    int UE_scan,
                    int UE_scan_carrier,
                    runmode_t mode,
                    int rxgain,
                    int txpowermax,
                    LTE_DL_FRAME_PARMS *fp);

extern void init_thread(int sched_runtime, int sched_deadline, int sched_fifo, cpu_set_t *cpuset, char *name);

extern void reset_opp_meas(void);
extern void print_opp_meas(void);

extern void init_td_thread(PHY_VARS_eNB *);
extern void init_te_thread(PHY_VARS_eNB *);
extern void kill_td_thread(PHY_VARS_eNB *);
extern void kill_te_thread(PHY_VARS_eNB *);

extern void init_ocm(void);
extern void init_ue_devices(PHY_VARS_UE *);

PHY_VARS_UE *init_ue_vars(LTE_DL_FRAME_PARMS *frame_parms, uint8_t UE_id, uint8_t abstraction_flag);

void init_eNB_afterRU(void);
extern int stop_L1L2(module_id_t enb_id);
extern int restart_L1L2(module_id_t enb_id);

extern void init_UE_stub_single_thread(int nb_inst, int eMBMS_active, int uecap_xer_in, char *emul_iface);

extern PHY_VARS_UE *init_ue_vars(LTE_DL_FRAME_PARMS *frame_parms, uint8_t UE_id, uint8_t abstraction_flag);

#endif
