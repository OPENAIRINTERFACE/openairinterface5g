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
//#undef MALLOC
#include "assertions.h"
#include "msc.h"
#include "PHY/types.h"
#include "PHY/defs_eNB.h"
#include "PHY/defs_UE.h"
#include "SIMULATION/ETH_TRANSPORT/proto.h"

#include "flexran_agent.h"
#include "s1ap_eNB.h"
#include "SIMULATION/ETH_TRANSPORT/proto.h"
#include "proto_agent.h"

/* help strings definition for command line options, used in CMDLINE_XXX_DESC macros and printed when -h option is used */
#define CONFIG_HLP_RFCFGF        "Configuration file for front-end (e.g. LMS7002M)\n"
#define CONFIG_HLP_ULMAXE        "set the eNodeB max ULSCH erros\n"
#define CONFIG_HLP_CALUER        "set UE RX calibration\n"
#define CONFIG_HLP_CALUERM       ""
#define CONFIG_HLP_CALUERB       ""
#define CONFIG_HLP_DBGUEPR       "UE run normal prach power ramping, but don't continue random-access\n"
#define CONFIG_HLP_CALPRACH      "UE run normal prach with maximum power, but don't continue random-access\n"
#define CONFIG_HLP_NOL2CN        "bypass L2 and upper layers\n"
#define CONFIG_HLP_SIML1         "activate RF simulator instead of HW\n"
#define CONFIG_HLP_NUMUE         "number of UE instances\n"
#define CONFIG_HLP_UERXG         "set UE RX gain\n"
#define CONFIG_HLP_UERXGOFF      "external UE amplifier offset\n"
#define CONFIG_HLP_UETXG         "set UE TX gain\n"
#define CONFIG_HLP_UENANTR       "set UE number of rx antennas\n"
#define CONFIG_HLP_UENANTT       "set UE number of tx antennas\n"
#define CONFIG_HLP_UESCAN        "set UE to scan around carrier\n"
#define CONFIG_HLP_DUMPFRAME     "dump UE received frame to rxsig_frame0.dat and exit\n"
#define CONFIG_HLP_DLSHIFT       "dynamic shift for LLR compuation for TM3/4 (default 0)\n"
#define CONFIG_HLP_UELOOP        "get softmodem (UE) to loop through memory instead of acquiring from HW\n"
#define CONFIG_HLP_PHYTST        "test UE phy layer, mac disabled\n"
#define CONFIG_HLP_DMAMAP        "sets flag for improved EXMIMO UE performance\n"
#define CONFIG_HLP_EXTS          "tells hardware to use an external timing reference\n"
#define CONFIG_HLP_DMRSSYNC      "tells RU to insert DMRS in subframe 1 slot 0"
#define CONFIG_HLP_CLK           "tells hardware to use a clock reference (0:internal, 1:external, 2:gpsdo)\n"
#define CONFIG_HLP_USIM          "use XOR autentication algo in case of test usim mode\n"
#define CONFIG_HLP_NOSNGLT       "Disables single-thread mode in lte-softmodem\n"
#define CONFIG_HLP_TADV          "Set timing_advance\n"
#define CONFIG_HLP_DLF           "Set the downlink frequency for all component carriers\n"
#define CONFIG_HLP_CHOFF         "Channel id offset\n"
#define CONFIG_HLP_SOFTS         "Enable soft scope and L1 and L2 stats (Xforms)\n"
#define CONFIG_HLP_EXMCAL        "Calibrate the EXMIMO borad, available files: exmimo2_2arxg.lime exmimo2_2brxg.lime \n"
#define CONFIG_HLP_ITTIL         "Generate ITTI analyzser logs (similar to wireshark logs but with more details)\n"
#define CONFIG_HLP_DLMCS         "Set the maximum downlink MCS\n"
#define CONFIG_HLP_STMON         "Enable processing timing measurement of lte softmodem on per subframe basis \n"
#define CONFIG_HLP_PRB           "Set the PRB, valid values: 6, 25, 50, 100  \n"
#define CONFIG_HLP_EMULIFACE     "Set the interface name for the multicast transport for emulation mode (e.g. eth0, lo, etc.)  \n"
//#define CONFIG_HLP_NUMUES    "Set the number of UEs for the emulation"
#define CONFIG_HLP_MSLOTS        "Skip the missed slots/subframes \n"
#define CONFIG_HLP_ULMCS         "Set the maximum uplink MCS\n"
#define CONFIG_HLP_TDD           "Set hardware to TDD mode (default: FDD). Used only with -U (otherwise set in config file).\n"
#define CONFIG_HLP_UE            "Set the lte softmodem as a UE\n"
#define CONFIG_HLP_TQFS          "Apply three-quarter of sampling frequency, 23.04 Msps to reduce the data rate on USB/PCIe transfers (only valid for 20 MHz)\n"
#define CONFIG_HLP_TPORT         "tracer port\n"
#define CONFIG_HLP_NOTWAIT       "don't wait for tracer, start immediately\n"
#define CONFIG_HLP_TNOFORK       "to ease debugging with gdb\n"

#define CONFIG_HLP_NUMEROLOGY    "adding numerology for 5G\n"
#define CONFIG_HLP_EMULATE_RF    "Emulated RF enabled(disable by defult)\n"
#define CONFIG_HLP_PARALLEL_CMD  "three config for level of parallelism 'PARALLEL_SINGLE_THREAD', 'PARALLEL_RU_L1_SPLIT', or 'PARALLEL_RU_L1_TRX_SPLIT'\n"
#define CONFIG_HLP_WORKER_CMD    "two option for worker 'WORKER_DISABLE' or 'WORKER_ENABLE'\n"
#define CONFIG_HLP_NOS1          "Disable s1 interface\n"
#define CONFIG_HLP_RFSIM         "Run in rf simulator mode (also known as basic simulator)\n"
#define CONFIG_HLP_NOKRNMOD      "(noS1 only): Use tun instead of namesh module \n"
#define CONFIG_HLP_DISABLNBIOT   "disable nb-iot, even if defined in config\n"

#define CONFIG_HLP_USRP_ARGS                "set the arguments to identify USRP (same syntax as in UHD)\n"
#define CONFIG_HLP_USRP_CLK_SRC              "USRP clock source: 'internal' or 'external'\n"

/***************************************************************************************************************************************/
/* command line options definitions, CMDLINE_XXXX_DESC macros are used to initialize paramdef_t arrays which are then used as argument
   when calling config_get or config_getlist functions                                                                                 */


/*------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters defining UE running mode                                              */
/*   optname                     helpstr                paramflags                      XXXptr        defXXXval         type       numelt   */
/*------------------------------------------------------------------------------------------------------------------------------------------*/
#define CMDLINE_UEMODEPARAMS_DESC {  \
    {"calib-ue-rx",                 CONFIG_HLP_CALUER,     0,    iptr:&rx_input_level_dBm,   defintval:0,  TYPE_INT,   0},    \
    {"calib-ue-rx-med",             CONFIG_HLP_CALUERM,    0,    iptr:&rx_input_level_dBm,   defintval:0,  TYPE_INT,   0},    \
    {"calib-ue-rx-byp",             CONFIG_HLP_CALUERB,    0,    iptr:&rx_input_level_dBm,   defintval:0,  TYPE_INT,   0},    \
    {"debug-ue-prach",              CONFIG_HLP_DBGUEPR,    PARAMFLAG_BOOL,   uptr:NULL,        defuintval:1,   TYPE_INT,   0},    \
    {"no-L2-connect",               CONFIG_HLP_NOL2CN,     PARAMFLAG_BOOL,   uptr:NULL,        defuintval:1,   TYPE_INT,   0},    \
    {"calib-prach-tx",              CONFIG_HLP_CALPRACH,   PARAMFLAG_BOOL,   uptr:NULL,        defuintval:1,   TYPE_INT,   0},    \
    {"loop-memory",                 CONFIG_HLP_UELOOP,     0,    strptr:&loopfile,       defstrval:"iqs.in", TYPE_STRING,0},    \
    {"ue-dump-frame",               CONFIG_HLP_DUMPFRAME,  PARAMFLAG_BOOL,   iptr:&dumpframe,      defintval:0,  TYPE_INT,   0},    \
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

#define START_MSC          softmodem_params.start_msc
/*-------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters specific to UE                                                                     */
/*   optname                     helpstr             paramflags                      XXXptr                  defXXXval            type          numelt   */
/*-------------------------------------------------------------------------------------------------------------------------------------------------------*/

#define CMDLINE_UEPARAMS_DESC {  \
    {"siml1",                      CONFIG_HLP_SIML1,       PARAMFLAG_BOOL,  iptr:&simL1flag,                    defintval:0,          TYPE_INT,      0},   \
    {"U",            CONFIG_HLP_NUMUE,       0,               u16ptr:&NB_UE_INST,                  defuintval:1,         TYPE_UINT16,     0},   \
    {"ue-rxgain",                CONFIG_HLP_UERXG,       0,   dblptr:&(rx_gain[0][0]),      defdblval:130,        TYPE_DOUBLE,   0},   \
    {"ue-rxgain-off",            CONFIG_HLP_UERXGOFF,    0,   dblptr:&rx_gain_off,        defdblval:0,          TYPE_DOUBLE,   0},   \
    {"ue-txgain",                CONFIG_HLP_UETXG,       0,   dblptr:&(tx_gain[0][0]),      defdblval:0,          TYPE_DOUBLE,   0},   \
    {"ue-nb-ant-rx",             CONFIG_HLP_UENANTR,     0,   u8ptr:&nb_antenna_rx,       defuintval:1,         TYPE_UINT8,    0},   \
    {"ue-nb-ant-tx",             CONFIG_HLP_UENANTT,     0,   u8ptr:&nb_antenna_tx,       defuintval:1,         TYPE_UINT8,    0},   \
    {"ue-scan-carrier",          CONFIG_HLP_UESCAN,      PARAMFLAG_BOOL,  iptr:&UE_scan_carrier,        defintval:0,          TYPE_INT,  0},   \
    {"ue-max-power",             NULL,           0,   iptr:&(tx_max_power[0]),      defintval:23,         TYPE_INT,  0},   \
    {"emul-iface",                 CONFIG_HLP_EMULIFACE,   0,               strptr:&emul_iface,       defstrval:"lo",       TYPE_STRING,   100}, \
    {"L2-emul",                    NULL,             0,               u8ptr:&nfapi_mode,        defuintval:3,         TYPE_UINT8,  0},   \
    {"num-ues",              NULL,                   0,               u16ptr:&(NB_UE_INST),        defuintval:1,         TYPE_UINT16,  0},   \
    {"nums_ue_thread",             NULL,                   0,               u16ptr:&(NB_THREAD_INST),           defuintval:1,         TYPE_UINT16,   0},   \
    {"r"  ,                        CONFIG_HLP_PRB,         0,               u8ptr:&(frame_parms[0]->N_RB_DL),   defintval:25,         TYPE_UINT8,    0},   \
    {"dlsch-demod-shift",          CONFIG_HLP_DLSHIFT,     0,   iptr:(int32_t *)&dlsch_demod_shift, defintval:0,          TYPE_INT,  0},   \
    {"usrp-args",                  CONFIG_HLP_USRP_ARGS,   0,               strptr:(char **)&usrp_args,         defstrval:"type=b200",TYPE_STRING,   0},   \
    {"usrp-clksrc",                CONFIG_HLP_USRP_CLK_SRC,0,               strptr:(char **)&usrp_clksrc,       defstrval:"internal", TYPE_STRING,   0},   \
    {"mmapped-dma",                CONFIG_HLP_DMAMAP,      PARAMFLAG_BOOL,  uptr:&mmapped_dma,                  defintval:0,          TYPE_INT,      0},   \
    {"clock",                      CONFIG_HLP_CLK,         0,               uptr:&clock_source,                 defintval:0,          TYPE_UINT,     0},   \
    {"T" ,                         CONFIG_HLP_TDD,         PARAMFLAG_BOOL,  iptr:&tddflag,                      defintval:0,          TYPE_INT,      0},   \
    {"A",                          CONFIG_HLP_TADV,        0,               iptr:&(timingadv),                  defintval:0,          TYPE_INT,      0}    \
  }

/*-------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters specific to UE threads                                                             */
/*   optname                     helpstr             paramflags                      XXXptr                  defXXXval            type          numelt   */
/*-------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CMDLINE_UETHREADSPARAMS_DESC {  \
    {"threadIQ",                NULL,                   0,                      iptr:&(threads.iq),                 defintval:1,                    TYPE_INT,       0},                     \
    {"threadOneSubframe",       NULL,                   0,                      iptr:&(threads.one),                defintval:1,                    TYPE_INT,       0},                     \
    {"threadTwoSubframe",       NULL,                   0,                      iptr:&(threads.two),                defintval:1,                    TYPE_INT,       0},                     \
    {"threadThreeSubframe",     NULL,                   0,                      iptr:&(threads.three),              defintval:1,                    TYPE_INT,       0},                     \
    {"threadSlot1ProcOne",      NULL,                   0,                      iptr:&(threads.slot1_proc_one),     defintval:1,                    TYPE_INT,       0},                     \
    {"threadSlot1ProcTwo",      NULL,                   0,                      iptr:&(threads.slot1_proc_two),     defintval:1,                    TYPE_INT,       0},                     \
  }

#define DEFAULT_DLF 2680000000

/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters common to eNodeB and UE                                                                                */
/*   optname                     helpstr                paramflags                      XXXptr                  defXXXval                            type           numelt   */
/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define RF_CONFIG_FILE      softmodem_params.rf_config_file
#define PHY_TEST            softmodem_params.phy_test
#define WAIT_FOR_SYNC       softmodem_params.wait_for_sync
#define SINGLE_THREAD_FLAG  softmodem_params.single_thread_flag
#define CHAIN_OFFSET        softmodem_params.chain_offset
#define NUMEROLOGY          softmodem_params.numerology
#define EMULATE_RF          softmodem_params.emulate_rf
#define CLOCK_SOURCE        softmodem_params.clock_source
#define TIMING_SOURCE       softmodem_params.timing_source
#define SEND_DMRSSYNC       softmodem_params.send_dmrs_sync
#define USIM_TEST           softmodem_params.usim_test
#define CMDLINE_PARAMS_DESC {  \
    {"rf-config-file",          CONFIG_HLP_RFCFGF,      0,                      strptr:(char **)&RF_CONFIG_FILE,    defstrval:NULL,                 TYPE_STRING,    sizeof(RF_CONFIG_FILE)}, \
    {"ulsch-max-errors",        CONFIG_HLP_ULMAXE,      0,                      uptr:&ULSCH_max_consecutive_errors, defuintval:0,                   TYPE_UINT,      0},                     \
    {"phy-test",                CONFIG_HLP_PHYTST,      PARAMFLAG_BOOL,         iptr:&PHY_TEST,                     defintval:0,                    TYPE_INT,       0},                     \
    {"usim-test",               CONFIG_HLP_USIM,        PARAMFLAG_BOOL,         u8ptr:&USIM_TEST,                   defintval:0,                    TYPE_UINT8,     0},                     \
    {"emulate-rf" ,             CONFIG_HLP_EMULATE_RF,  PARAMFLAG_BOOL,         iptr:&EMULATE_RF,                   defintval:0,                    TYPE_INT,       0},                     \
    {"clock",                   CONFIG_HLP_CLK,         0,                      uptr:&CLOCK_SOURCE,                 defintval:0,                    TYPE_UINT,      0},                     \
    {"wait-for-sync",           NULL,                   PARAMFLAG_BOOL,         iptr:&WAIT_FOR_SYNC,                defintval:0,                    TYPE_INT,       0},                     \
    {"single-thread-enable",    CONFIG_HLP_NOSNGLT,     PARAMFLAG_BOOL,         iptr:&SINGLE_THREAD_FLAG,           defintval:0,                    TYPE_INT,       0},                     \
    {"C" ,                      CONFIG_HLP_DLF,         0,                      uptr:&(downlink_frequency[0][0]),   defuintval:2680000000,          TYPE_UINT,      0},                     \
    {"a" ,                      CONFIG_HLP_CHOFF,       0,                      iptr:&CHAIN_OFFSET,                 defintval:0,                    TYPE_INT,       0},                     \
    {"d" ,                      CONFIG_HLP_SOFTS,       PARAMFLAG_BOOL,         uptr:(uint32_t *)&do_forms,         defintval:0,                    TYPE_INT8,      0},                     \
    {"q" ,                      CONFIG_HLP_STMON,       PARAMFLAG_BOOL,         iptr:&opp_enabled,                  defintval:0,                    TYPE_INT,       0},                     \
    {"S" ,                      CONFIG_HLP_MSLOTS,      PARAMFLAG_BOOL,         u8ptr:&exit_missed_slots,           defintval:1,                    TYPE_UINT8,     0},                     \
    {"numerology" ,             CONFIG_HLP_NUMEROLOGY,  PARAMFLAG_BOOL,         iptr:&NUMEROLOGY,                   defintval:0,                    TYPE_INT,       0},                     \
    {"parallel-config",         CONFIG_HLP_PARALLEL_CMD,0,                      strptr:(char **)&parallel_config,   defstrval:NULL,                 TYPE_STRING,    0},                     \
    {"worker-config",           CONFIG_HLP_WORKER_CMD,  0,                      strptr:(char **)&worker_config,     defstrval:NULL,                 TYPE_STRING,    0},                     \
    {"noS1",                    CONFIG_HLP_NOS1,        PARAMFLAG_BOOL,         uptr:&noS1,         defintval:0,      TYPE_INT, 0},                     \
    {"rfsim",                   CONFIG_HLP_RFSIM,       PARAMFLAG_BOOL,         uptr:&rfsim,        defintval:0,      TYPE_INT, 0},                     \
    {"basicsim",                CONFIG_HLP_RFSIM,       PARAMFLAG_BOOL,         uptr:&basicsim,     defintval:0,      TYPE_INT, 0},                     \
    {"nokrnmod",                CONFIG_HLP_NOKRNMOD,    PARAMFLAG_BOOL,         uptr:&nokrnmod,     defintval:0,      TYPE_INT, 0},                     \
    {"nbiot-disable",           CONFIG_HLP_DISABLNBIOT, PARAMFLAG_BOOL,         uptr:&nonbiot,      defuintval:0,                   TYPE_INT,       0},                     \
  }

#define CONFIG_HLP_FLOG          "Enable online log \n"
#define CONFIG_HLP_LOGL          "Set the global log level, valide options: (4:trace, 3:debug, 2:info, 1:warn, (0:error))\n"
#define CONFIG_HLP_TELN          "Start embedded telnet server \n"
#define CONFIG_HLP_MSC           "Enable the MSC tracing utility \n"
/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters for LOG utility                                                                                        */
/*   optname                     helpstr                paramflags                      XXXptr                  defXXXval                            type           numelt   */
/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define START_MSC          softmodem_params.start_msc
#define CMDLINE_LOGPARAMS_DESC {  \
    {"R" ,           CONFIG_HLP_FLOG, 0,                uptr:&online_log_messages,    defintval:1,         TYPE_INT,    0},        \
    {"g" ,           CONFIG_HLP_LOGL, 0,      uptr:&glog_level,     defintval:0,         TYPE_UINT,     0},        \
    {"telnetsrv",        CONFIG_HLP_TELN, PARAMFLAG_BOOL,   uptr:&start_telnetsrv,    defintval:0,         TYPE_UINT,     0},        \
    {"msc",                CONFIG_HLP_MSC,  PARAMFLAG_BOOL,   uptr:&START_MSC,            defintval:0,         TYPE_UINT,     0},                    \
    {"log-mem",      NULL,            0,      strptr:(char **)&logmem_filename,    defstrval:NULL,          TYPE_STRING,   0},    \
  }
#define CMDLINE_ONLINELOG_IDX     0
#define CMDLINE_GLOGLEVEL_IDX     1
#define CMDLINE_STARTTELN_IDX     2

/* check function for global log level */
#define CMDLINE_LOGPARAMS_CHECK_DESC { \
    { .s5= {NULL }} ,                                                 \
    { .s2=  { config_check_intrange,           {0,4}}},               \
    { .s5= {NULL }} ,                   \
    { .s5= {NULL }} ,                   \
    { .s5= {NULL }} ,                   \
  }

/***************************************************************************************************************************************/
/*  */
#include "threads_t.h"

#define SOFTMODEM_NOS1_BIT            (1<<0)
#define SOFTMODEM_NOKRNMOD_BIT        (1<<1)
#define SOFTMODEM_NONBIOT_BIT         (1<<2)
#define SOFTMODEM_RFSIM_BIT           (1<<10)
#define SOFTMODEM_BASICSIM_BIT        (1<<11)
#define SOFTMODEM_SIML1_BIT           (1<<12)
#define SOFTMODEM_DOFORMS_BIT         (1<<15)
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

#define IS_SOFTMODEM_NOS1            ( get_softmodem_optmask() & SOFTMODEM_NOS1_BIT)
#define IS_SOFTMODEM_NOKRNMOD        ( get_softmodem_optmask() & SOFTMODEM_NOKRNMOD_BIT)
#define IS_SOFTMODEM_NONBIOT         ( get_softmodem_optmask() & SOFTMODEM_NONBIOT_BIT)
#define IS_SOFTMODEM_RFSIM           ( get_softmodem_optmask() & SOFTMODEM_RFSIM_BIT)
#define IS_SOFTMODEM_BASICSIM        ( get_softmodem_optmask() & SOFTMODEM_BASICSIM_BIT)
#define IS_SOFTMODEM_SIML1           ( get_softmodem_optmask() & SOFTMODEM_SIML1_BIT)
#define IS_SOFTMODEM_DOFORMS         ( get_softmodem_optmask() & SOFTMODEM_DOFORMS_BIT)
extern uint64_t get_softmodem_optmask(void);
extern uint64_t set_softmodem_optmask(uint64_t bitmask);
extern void get_common_options(void);
extern softmodem_params_t *get_softmodem_params(void);
extern unsigned int is_nos1exec(char *exepath) ;
uint64_t get_pdcp_optmask(void);
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

// In lte-enb.c
extern void init_eNB(int single_thread_flag,int wait_for_sync);
extern void stop_eNB(int);
extern void kill_eNB_proc(int inst);

// In lte-ru.c
extern void init_RU(char *,clock_source_t clock_source,clock_source_t time_source,int send_dmrssync);
extern void stop_ru(RU_t *ru);
extern void init_ru_vnf(void);
extern void init_RU_proc(RU_t *ru);
extern void stop_RU(int nb_ru);
extern void kill_RU_proc(RU_t *ru);
extern void set_function_spec_param(RU_t *ru);

// In lte-ue.c
extern int setup_ue_buffers(PHY_VARS_UE **phy_vars_ue, openair0_config_t *openair0_cfg);
extern void fill_ue_band_info(void);

extern void init_UE(int nb_inst,int eMBMS_active, int uecap_xer_in, int timing_correction, int phy_test, int UE_scan, int UE_scan_carrier, runmode_t mode,int rxgain,int txpowermax,
                    LTE_DL_FRAME_PARMS *fp);
extern void init_thread(int sched_runtime, int sched_deadline, int sched_fifo, cpu_set_t *cpuset, char *name);

extern void reset_opp_meas(void);
extern void print_opp_meas(void);


extern void init_td_thread(PHY_VARS_eNB *);
extern void init_te_thread(PHY_VARS_eNB *);
extern void kill_td_thread(PHY_VARS_eNB *);
extern void kill_te_thread(PHY_VARS_eNB *);

extern void RCConfig_sim(void);
extern void init_ocm(void);
extern void init_ue_devices(PHY_VARS_UE *);

PHY_VARS_UE *init_ue_vars(LTE_DL_FRAME_PARMS *frame_parms,
                          uint8_t UE_id,
                          uint8_t abstraction_flag);
void init_eNB_afterRU(void);
PARALLEL_CONF_t get_thread_parallel_conf(void);
WORKER_CONF_t   get_thread_worker_conf(void);
void set_parallel_conf(char *parallel_conf);
void set_worker_conf(char *worker_conf);

extern int stop_L1L2(module_id_t enb_id);
extern int restart_L1L2(module_id_t enb_id);


extern void init_UE_stub_single_thread(int nb_inst,int eMBMS_active, int uecap_xer_in, char *emul_iface);

extern PHY_VARS_UE *init_ue_vars(LTE_DL_FRAME_PARMS *frame_parms,
                                 uint8_t UE_id,
                                 uint8_t abstraction_flag);

#endif
