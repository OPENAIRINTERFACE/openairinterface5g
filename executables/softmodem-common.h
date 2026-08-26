/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*!
 * \brief Top-level threads for eNodeB
 */
#ifndef SOFTMODEM_COMMON_H
#define SOFTMODEM_COMMON_H
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "common/config/config_load_configmodule.h"

/* help strings definition for command line options, used in CMDLINE_XXX_DESC macros and printed when -h option is used */
#define CONFIG_HLP_RFCFGF        "Configuration file for front-end (e.g. LMS7002M)\n"
#define CONFIG_HLP_TPOOL \
  "Thread pool configuration: \n\
  list of cores, comma separated (negative value is no core affinity)\n\
  example: -1,3 launches two working threads one floating, the second set on core 3\n\
  default 8 floating threads\n\
  use N for no pool (runs in calling thread) recommended with rfsim.\n"
#define CONFIG_HLP_CALUER        "set UE RX calibration\n"
#define CONFIG_HLP_CALUERM       ""
#define CONFIG_HLP_CALUERB       ""
#define CONFIG_HLP_DBGUEPR       "UE run normal prach power ramping, but don't continue random-access\n"
#define CONFIG_HLP_CALPRACH      "UE run normal prach with maximum power, but don't continue random-access\n"
#define CONFIG_HLP_NOL2CN        "bypass L2 and upper layers\n"


#define CONFIG_HLP_DUMPFRAME     "dump UE received frame to rxsig_frame0.dat and exit\n"
#define CONFIG_HLP_PHYTST        "test UE phy layer, mac disabled\n"
#define CONFIG_HLP_DORA          "test gNB  and UE with RA procedures\n"
#define CONFIG_HLP_SL_MODE       "sets the NR sidelink mode (0: not in sidelink mode, 1: in-coverage/gNB, 2: out-of-coverage/no gNB)\n"
#define CONFIG_HLP_CLK           "tells hardware to use a clock reference (0:internal, 1:external, 2:gpsdo)\n"
#define CONFIG_HLP_TME           "tells hardware to use a time reference (0:internal, 1:external, 2:gpsdo)\n"
#define CONFIG_HLP_TUNE_OFFSET   "LO tuning offset to use in Hz\n"
#define CONFIG_HLP_DLF           "Set the downlink frequency for all component carriers\n"
#define CONFIG_HLP_ULF           "Set the uplink frequency offset for all component carriers\n"
#define CONFIG_HLP_SLF           "Set the sidelink frequency for all component carriers\n"
#define CONFIG_HLP_CHOFF         "Channel id offset\n"
#define CONFIG_HLP_SOFTS         "Enable soft scope and L1 and L2 stats (Xforms)\n"
#define CONFIG_HLP_DLMCS         "Set the maximum downlink MCS\n"
#define CONFIG_HLP_STMON         "Enable processing timing measurement of lte softmodem on per subframe basis \n"
#define CONFIG_HLP_CHESTFREQ     "Set channel estimation type in frequency domain. 0-Linear interpolation (default). 1-PRB based averaging of channel estimates in frequency. \n"
#define CONFIG_HLP_CHESTTIME     "Set channel estimation type in time domain. 0-Symbols take estimates of the last preceding DMRS symbol (default). 1-Symbol based averaging of channel estimates in time. \n"
#define CONFIG_HLP_IMSCOPE       "Enable phy scope based on imgui and implot"
#define CONFIG_HLP_IMSCOPE_RECORD "Enable recording scope data to filesystem"

#define CONFIG_HLP_NONSTOP       "Go back to frame sync mode after 100 consecutive PBCH failures\n"

#define CONFIG_HLP_TQFS                                                                                                          \
  "Apply three-quarter of sampling frequency, (example 23.04 Msps for LTE 20MHz) to reduce the data rate on USB/PCIe transfers " \
  "(only valid for some bandwidths)\n"
#define CONFIG_HLP_TPORT         "tracer port\n"
#define CONFIG_HLP_NOTWAIT       "don't wait for tracer, start immediately\n"

#define CONFIG_HLP_NUMEROLOGY    "adding numerology for 5G\n"
#define CONFIG_HLP_BAND          "band index\n"
#define CONFIG_HLP_PARALLEL_CMD  "three config for level of parallelism 'PARALLEL_SINGLE_THREAD', 'PARALLEL_RU_L1_SPLIT', or 'PARALLEL_RU_L1_TRX_SPLIT'\n"
#define CONFIG_HLP_WORKER_CMD    "two option for worker 'WORKER_DISABLE' or 'WORKER_ENABLE'\n"
#define CONFIG_HLP_USRP_THREAD   "having extra thead for usrp tx\n"

#define CONFIG_HLP_NOS1          "Disable s1 interface\n"
#define CONFIG_HLP_RFSIM         "Run in rf simulator mode\n"
#define CONFIG_HLP_DISABLNBIOT   "disable nb-iot, even if defined in config\n"
#define CONFIG_HLP_USRP_THREAD   "having extra thead for usrp tx\n"
#define CONFIG_HLP_NFAPI         "Change the nFAPI mode for NR 'MONOLITHIC', 'PNF', 'VNF', 'AERIAL','UE_STUB_PNF','UE_STUB_OFFNET','STANDALONE_PNF'\n"
#define CONFIG_HLP_CONTINUOUS_TX "perform continuous transmission, even in TDD mode (to work around USRP issues)\n"
#define CONFIG_HLP_STATS_DISABLE "disable globally the stats generation and persistence"
#define CONFIG_HLP_NOITTI        "Do not start itti threads, call queue processing in place, inside the caller thread"
#define CONFIG_HLP_SYNC_REF      "UE acts a Sync Reference in Sidelink. 0-none 1-GNB 2-GNSS 4-localtiming\n"
#define CONFIG_HLP_TADV                                                                                                      \
  "Set RF board timing_advance to compensate fix delay inside the RF board between Rx and Tx timestamps (RF board internal " \
  "issues)\n"

/*-----------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters common to eNodeB and UE                                                          */
/*   optname                 helpstr                  paramflags      XXXptr                              defXXXval              type         numelt   */
/*-----------------------------------------------------------------------------------------------------------------------------------------------------*/
#define RF_CONFIG_FILE      softmodem_params.rf_config_file
#define TP_CONFIG           softmodem_params.threadPoolConfig
#define CONTINUOUS_TX       softmodem_params.continuous_tx
#define PHY_TEST            softmodem_params.phy_test
#define DO_RA               softmodem_params.do_ra
#define SL_MODE             softmodem_params.sl_mode
#define CHAIN_OFFSET        softmodem_params.chain_offset
#define NUMEROLOGY          softmodem_params.numerology
#define BAND                softmodem_params.band
#define CLOCK_SOURCE        softmodem_params.clock_source
#define TIMING_SOURCE       softmodem_params.timing_source
#define TUNE_OFFSET         softmodem_params.tune_offset
#define CHEST_FREQ          softmodem_params.chest_freq
#define CHEST_TIME          softmodem_params.chest_time
#define NFAPI               softmodem_params.nfapi
#define NSA                 softmodem_params.nsa
#define NODE_NUMBER         softmodem_params.node_number
#define NON_STOP            softmodem_params.non_stop
#define CONTINUOUS_TX       softmodem_params.continuous_tx
#define SYNC_REF            softmodem_params.sync_ref
#define DEFAULT_PDU_ID      softmodem_params.default_pdu_session_id

extern int usrp_tx_thread;
// clang-format off
#define CMDLINE_PARAMS_DESC {  \
  {"rf-config-file",        CONFIG_HLP_RFCFGF,        0,              .strptr=&RF_CONFIG_FILE,                .defstrval=NULL,          TYPE_STRING, 0},  \
  {"thread-pool",           CONFIG_HLP_TPOOL,         0,              .strptr=&TP_CONFIG,                     .defstrval="-1,-1,-1,-1,-1,-1,-1,-1",  TYPE_STRING, 0},     \
  {"phy-test",              CONFIG_HLP_PHYTST,        PARAMFLAG_BOOL, .iptr=&PHY_TEST,                        .defintval=0,             TYPE_INT,    0},  \
  {"do-ra",                 CONFIG_HLP_DORA,          PARAMFLAG_BOOL, .iptr=&DO_RA,                           .defintval=0,             TYPE_INT,    0},  \
  {"sl-mode",               CONFIG_HLP_SL_MODE,       0,              .u8ptr=&SL_MODE,                        .defintval=0,             TYPE_UINT8,  0},  \
  {"clock-source",          CONFIG_HLP_CLK,           0,              .uptr=&CLOCK_SOURCE,                    .defintval=0,             TYPE_UINT,   0},  \
  {"time-source",           CONFIG_HLP_TME,           0,              .uptr=&TIMING_SOURCE,                   .defintval=0,             TYPE_UINT,   0},  \
  {"tune-offset",           CONFIG_HLP_TUNE_OFFSET,   0,              .dblptr=&TUNE_OFFSET,                   .defintval=0,             TYPE_DOUBLE, 0},  \
  {"C" ,                    CONFIG_HLP_DLF,           0,              .u64ptr=&(downlink_frequency[0][0]),    .defuintval=0,            TYPE_UINT64, 0},  \
  {"CO" ,                   CONFIG_HLP_ULF,           0,              .i64ptr=&(uplink_frequency_offset[0][0]), .defintval=0,           TYPE_INT64,  0},  \
  {"a" ,                    CONFIG_HLP_CHOFF,         0,              .iptr=&CHAIN_OFFSET,                    .defintval=0,             TYPE_INT,    0},  \
  {"d" ,                    CONFIG_HLP_SOFTS,         PARAMFLAG_BOOL, .uptr=&do_forms,                        .defintval=0,             TYPE_UINT,   0},  \
  {"q" ,                    CONFIG_HLP_STMON,         PARAMFLAG_VALUE_OPTIONAL, .iptr=&cpu_meas_enabled,      .defintval=0,             TYPE_INT,    0},  \
  {"numerology" ,           CONFIG_HLP_NUMEROLOGY,    0,              .iptr=&NUMEROLOGY,                      .defintval=1,             TYPE_INT,    0},  \
  {"band" ,                 CONFIG_HLP_BAND,          0,              .iptr=&BAND,                            .defintval=78,            TYPE_INT,    0},  \
  {"parallel-config",       CONFIG_HLP_PARALLEL_CMD,  0,              .strptr=&parallel_config,               .defstrval=NULL,          TYPE_STRING, 0},  \
  {"worker-config",         CONFIG_HLP_WORKER_CMD,    0,              .strptr=&worker_config,                 .defstrval=NULL,          TYPE_STRING, 0},  \
  {"noS1",                  CONFIG_HLP_NOS1,          PARAMFLAG_BOOL, .uptr=&noS1,                            .defintval=0,             TYPE_UINT,   0},  \
  {"rfsim",                 CONFIG_HLP_RFSIM,         PARAMFLAG_BOOL, .uptr=&rfsim,                           .defintval=0,             TYPE_UINT,   0},  \
  {"nbiot-disable",         CONFIG_HLP_DISABLNBIOT,   PARAMFLAG_BOOL, .uptr=&nonbiot,                         .defuintval=0,            TYPE_UINT,   0},  \
  {"chest-freq",            CONFIG_HLP_CHESTFREQ,     0,              .iptr=&CHEST_FREQ,                      .defintval=0,             TYPE_INT,    0},  \
  {"chest-time",            CONFIG_HLP_CHESTTIME,     0,              .iptr=&CHEST_TIME,                      .defintval=0,             TYPE_INT,    0},  \
  {"nsa",                   CONFIG_HLP_NSA,           PARAMFLAG_BOOL, .iptr=&NSA,                             .defintval=0,             TYPE_INT,    0},  \
  {"node-number",           NULL,                     0,              .u16ptr=&NODE_NUMBER,                   .defuintval=0,            TYPE_UINT16, 0},  \
  {"usrp-tx-thread-config", CONFIG_HLP_USRP_THREAD,   0,              .iptr=&usrp_tx_thread,                  .defstrval=0,             TYPE_INT,    0},  \
  {"nfapi",                 CONFIG_HLP_NFAPI,         0,              .strptr=NULL,                           .defstrval="MONOLITHIC",  TYPE_STRING, 0},  \
  {"non-stop",              CONFIG_HLP_NONSTOP,       PARAMFLAG_BOOL, .iptr=&NON_STOP,                        .defintval=0,             TYPE_INT,    0},  \
  {"continuous-tx",         CONFIG_HLP_CONTINUOUS_TX, PARAMFLAG_BOOL, .iptr=&CONTINUOUS_TX,                   .defintval=0,             TYPE_INT,    0},  \
  {"disable-stats",         CONFIG_HLP_STATS_DISABLE, PARAMFLAG_BOOL, .iptr=&stats_disabled,                  .defintval=0,             TYPE_INT,    0},  \
  {"no-itti-threads",       CONFIG_HLP_NOITTI,        PARAMFLAG_BOOL, .iptr=&softmodem_params.no_itti,        .defintval=0,             TYPE_INT,    0},  \
  {"sync-ref",              CONFIG_HLP_SYNC_REF,      0,              .uptr=&SYNC_REF,                        .defintval=0,             TYPE_UINT,   0},  \
  {"A" ,                    CONFIG_HLP_TADV,          0,             .iptr=&softmodem_params.command_line_sample_advance,.defintval=0,            TYPE_INT,   0},  \
  {"E" ,                    CONFIG_HLP_TQFS,          PARAMFLAG_BOOL, .iptr=&softmodem_params.threequarter_fs, .defintval=0,            TYPE_INT,    0}, \
  {"imscope" ,              CONFIG_HLP_IMSCOPE,       PARAMFLAG_BOOL, .uptr=&enable_imscope,                   .defintval=0,            TYPE_UINT,   0}, \
  {"imscope-record" ,       CONFIG_HLP_IMSCOPE_RECORD,PARAMFLAG_BOOL, .uptr=&enable_imscope_record,            .defintval=0,            TYPE_UINT,   0}, \
  {"default-pdu-id",        NULL,                     0,              .iptr=&DEFAULT_PDU_ID,                   .defintval=-1,           TYPE_INT,    0}, \
}
// clang-format on

// clang-format off
#define CMDLINE_PARAMS_CHECK_DESC {         \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s3a = { config_checkstr_assign_integer, \
               {"MONOLITHIC", "PNF", "VNF", "AERIAL","UE_STUB_PNF","UE_STUB_OFFNET","STANDALONE_PNF"}, \
               {NFAPI_MONOLITHIC, NFAPI_MODE_PNF, NFAPI_MODE_VNF, NFAPI_MODE_AERIAL,NFAPI_UE_STUB_PNF,NFAPI_UE_STUB_OFFNET,NFAPI_MODE_STANDALONE_PNF}, \
               7 } }, \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
}
// clang-format on

#define CONFIG_HLP_NSA           "Enable NSA mode \n"
#define CONFIG_HLP_FLOG          "Enable online log \n"
#define CONFIG_HLP_LOGL          "Set the global log level, valid options: (4:trace, 3:debug, 2:info, 1:warn, (0:error))\n"
#define CONFIG_HLP_TELN          "Start embedded telnet server \n"
#define CONFIG_HLP_WEB "Start embedded web server \n"
#define CONFIG_FLOG_OPT          "R"
#define CONFIG_LOGL_OPT          "g"
/*-------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters for LOG utility                                                              */
/*   optname                        helpstr       paramflags        XXXptr                              defXXXval            type           numelt */
/*-------------------------------------------------------------------------------------------------------------------------------------------------*/
// clang-format off
#define CMDLINE_LOGPARAMS_DESC  { \
  {CONFIG_FLOG_OPT, CONFIG_HLP_FLOG, 0,                                      .uptr = &online_log_messages, .defintval = 1,    TYPE_INT,    0}, \
  {CONFIG_LOGL_OPT, CONFIG_HLP_LOGL, 0,                                      .uptr = &glog_level,          .defintval = 0,    TYPE_UINT,   0}, \
  {"telnetsrv",     CONFIG_HLP_TELN, PARAMFLAG_BOOL | PARAMFLAG_CMDLINEONLY, .uptr = &start_telnetsrv,     .defintval = 0,    TYPE_UINT,   0}, \
  {"websrv",        CONFIG_HLP_WEB,  PARAMFLAG_BOOL | PARAMFLAG_CMDLINEONLY, .uptr = &start_websrv,        .defintval = 0,    TYPE_UINT,   0}, \
  {"log-mem",       NULL,            0,                                      .strptr = &logmem_filename,   .defstrval = NULL, TYPE_STRING, 0}, \
  {"telnetclt",     NULL,            0,                                      .uptr = &start_telnetclt,     .defstrval = NULL, TYPE_UINT,   0}, \
}
// clang-format on

/* check function for global log level */
// clang-format off
#define CMDLINE_LOGPARAMS_CHECK_DESC { \
  { .s5= {NULL} } ,                       \
  { .s2= {config_check_intrange, {0,4}}}, \
  { .s5= {NULL} } ,                       \
  { .s5= {NULL} } ,                       \
  { .s5= {NULL} } ,                       \
  { .s5= {NULL} } ,                       \
}
// clang-format on

/***************************************************************************************************************************************/

#define IS_SOFTMODEM_NOS1 (get_softmodem_optmask()->bit.SOFTMODEM_NOS1_BIT)
#define IS_SOFTMODEM_NONBIOT (get_softmodem_optmask()->bit.SOFTMODEM_NONBIOT_BIT)
#define IS_SOFTMODEM_RFSIM (get_softmodem_optmask()->bit.SOFTMODEM_RFSIM_BIT)
#define IS_SOFTMODEM_SIML1 (get_softmodem_optmask()->bit.SOFTMODEM_SIML1_BIT)
#define IS_SOFTMODEM_DLSIM (get_softmodem_optmask()->bit.SOFTMODEM_DLSIM_BIT)
#define IS_SOFTMODEM_DOSCOPE (get_softmodem_optmask()->bit.SOFTMODEM_DOSCOPE_BIT)
#define IS_SOFTMODEM_IQPLAYER (get_softmodem_optmask()->bit.SOFTMODEM_RECPLAY_BIT)
#define IS_SOFTMODEM_IQRECORDER (get_softmodem_optmask()->bit.SOFTMODEM_RECRECORD_BIT)
#define IS_SOFTMODEM_TELNETCLT (get_softmodem_optmask()->bit.SOFTMODEM_TELNETCLT_BIT)
#define IS_SOFTMODEM_ENB (get_softmodem_optmask()->bit.SOFTMODEM_ENB_BIT)
#define IS_SOFTMODEM_GNB (get_softmodem_optmask()->bit.SOFTMODEM_GNB_BIT)
#define IS_SOFTMODEM_4GUE (get_softmodem_optmask()->bit.SOFTMODEM_4GUE_BIT)
#define IS_SOFTMODEM_5GUE (get_softmodem_optmask()->bit.SOFTMODEM_5GUE_BIT)
#define IS_SOFTMODEM_NOSTATS (get_softmodem_optmask()->bit.SOFTMODEM_NOSTATS_BIT)
#define IS_SOFTMODEM_IMSCOPE_ENABLED (get_softmodem_optmask()->bit.SOFTMODEM_IMSCOPE_BIT)
#define IS_SOFTMODEM_IMSCOPE_RECORD_ENABLED (get_softmodem_optmask()->bit.SOFTMODEM_IMSCOPE_RECORD_BIT)
typedef struct optmask_s {
  union {
    struct {
      uint64_t SOFTMODEM_NOS1_BIT: 1;
      uint64_t SOFTMODEM_NOKRNMOD_BIT: 1;
      uint64_t SOFTMODEM_NONBIOT_BIT: 1;
      uint64_t SOFTMODEM_RFSIM_BIT: 1;
      uint64_t SOFTMODEM_SIML1_BIT: 1;
      uint64_t SOFTMODEM_DLSIM_BIT: 1;
      uint64_t SOFTMODEM_DOSCOPE_BIT: 1;
      uint64_t SOFTMODEM_RECPLAY_BIT: 1;
      uint64_t SOFTMODEM_TELNETCLT_BIT: 1;
      uint64_t SOFTMODEM_RECRECORD_BIT: 1;
      uint64_t SOFTMODEM_ENB_BIT: 1;
      uint64_t SOFTMODEM_GNB_BIT: 1;
      uint64_t SOFTMODEM_4GUE_BIT: 1;
      uint64_t SOFTMODEM_5GUE_BIT: 1;
      uint64_t SOFTMODEM_NOSTATS_BIT: 1;
      uint64_t SOFTMODEM_IMSCOPE_BIT: 1;
      uint64_t SOFTMODEM_IMSCOPE_RECORD_BIT : 1;
    } bit;
    uint64_t v; // allow to export entire bit set, force to 64 bit processor atomic size
  };
} optmask_t;
typedef struct {
  optmask_t optmask;
  //THREAD_STRUCT  thread_struct;
  char           *rf_config_file;
  char *threadPoolConfig;
  int            phy_test;
  int            do_ra;
  uint8_t        sl_mode;
  int            chain_offset;
  int            numerology;
  int            band;
  uint32_t       clock_source;
  uint32_t       timing_source;
  double         tune_offset;
  int command_line_sample_advance;
  uint32_t       send_dmrs_sync;
  int            use_256qam_table;
  int            chest_time;
  int            chest_freq;
  uint8_t        nfapi;
  int            nsa;
  uint16_t       node_number;
  int            non_stop;
  int            continuous_tx;
  uint32_t       sync_ref;
  int no_itti;
  int threequarter_fs;
  int default_pdu_session_id;
  int extra_pdu_session_id;
} softmodem_params_t;

#define IS_SA_MODE(sM_params) (!(sM_params)->phy_test && !(sM_params)->do_ra && !(sM_params)->nsa)
void softmodem_verify_mode(const softmodem_params_t *p);

#define get_softmodem_optmask() (&(get_softmodem_params()->optmask))
softmodem_params_t *get_softmodem_params(void);
void get_common_options(configmodule_interface_t *cfg);
char *get_softmodem_function(void);
#define SOFTMODEM_RTSIGNAL  (SIGRTMIN+1)
void set_softmodem_sighandler(void);
extern uint64_t downlink_frequency[MAX_NUM_CCs][4];
extern int64_t uplink_frequency_offset[MAX_NUM_CCs][4];
extern int usrp_tx_thread;
extern int sf_ahead;
extern int oai_exit;

void configure_ru(void *, void *arg);
void configure_rru(void *, void *arg);
struct timespec timespec_add(struct timespec lhs, struct timespec rhs);
struct timespec timespec_sub(struct timespec lhs, struct timespec rhs);
extern uint8_t nfapi_mode;
extern char *parallel_config;
extern char *worker_config;
extern double cpuf;

#ifdef __cplusplus
}
#endif
#endif
