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

/*! \file softmodem-common.h
 * \brief Top-level threads for eNodeB
 * \author
 * \date 2012
 * \version 0.1
 * \company Eurecom
 * \email:
 * \note
 * \warning
 */
#ifndef SOFTMODEM_COMMON_H
#define SOFTMODEM_COMMON_H
#include "openair1/PHY/defs_common.h"
#ifdef __cplusplus
extern "C"
{
#endif
/* help strings definition for command line options, used in CMDLINE_XXX_DESC macros and printed when -h option is used */
#define CONFIG_HLP_RFCFGF        "Configuration file for front-end (e.g. LMS7002M)\n"
#define CONFIG_HLP_SPLIT73       "Split 7.3 (below rate matching) option: <cu|du>:<remote ip address>:<remote port>\n"
#define CONFIG_HLP_TPOOL         "Thread pool configuration: \n\
  list of cores, comma separated (negative value is no core affinity)\n\
  example: -1,3 launches two working threads one floating, the second set on core 3\n\
  default 8 floating threads\n\
  use N for no pool (runs in calling thread) recommended with rfsim.\n"
#define CONFIG_HLP_ULMAXE        "set the eNodeB max ULSCH erros\n"
#define CONFIG_HLP_CALUER        "set UE RX calibration\n"
#define CONFIG_HLP_CALUERM       ""
#define CONFIG_HLP_CALUERB       ""
#define CONFIG_HLP_DBGUEPR       "UE run normal prach power ramping, but don't continue random-access\n"
#define CONFIG_HLP_CALPRACH      "UE run normal prach with maximum power, but don't continue random-access\n"
#define CONFIG_HLP_NOL2CN        "bypass L2 and upper layers\n"


#define CONFIG_HLP_DUMPFRAME     "dump UE received frame to rxsig_frame0.dat and exit\n"
#define CONFIG_HLP_PHYTST        "test UE phy layer, mac disabled\n"
#define CONFIG_HLP_DORA          "test gNB  and UE with RA procedures\n"
#define CONFIG_HLP_SA            "run gNB in standalone mode\n"
#define CONFIG_HLP_SL_MODE       "sets the NR sidelink mode (0: not in sidelink mode, 1: in-coverage/gNB, 2: out-of-coverage/no gNB)\n"
#define CONFIG_HLP_EXTS          "tells hardware to use an external timing reference\n"
#define CONFIG_HLP_DMRSSYNC      "tells RU to insert DMRS in subframe 1 slot 0"
#define CONFIG_HLP_CLK           "tells hardware to use a clock reference (0:internal, 1:external, 2:gpsdo)\n"
#define CONFIG_HLP_TME           "tells hardware to use a time reference (0:internal, 1:external, 2:gpsdo)\n"
#define CONFIG_HLP_TUNE_OFFSET   "LO tuning offset to use in Hz\n"
#define CONFIG_HLP_USIM          "use XOR autentication algo in case of test usim mode\n"
#define CONFIG_HLP_NOSNGLT       "Disables single-thread mode in lte-softmodem\n"
#define CONFIG_HLP_DLF           "Set the downlink frequency for all component carriers\n"
#define CONFIG_HLP_ULF           "Set the uplink frequency offset for all component carriers\n"
#define CONFIG_HLP_SLF           "Set the sidelink frequency for all component carriers\n"
#define CONFIG_HLP_CHOFF         "Channel id offset\n"
#define CONFIG_HLP_SOFTS         "Enable soft scope and L1 and L2 stats (Xforms)\n"
#define CONFIG_HLP_SOFTS_QT      "Enable soft scope and L1 and L2 stats (QT)\n"
#define CONFIG_HLP_ITTIL         "Generate ITTI analyzser logs (similar to wireshark logs but with more details)\n"
#define CONFIG_HLP_DLMCS         "Set the maximum downlink MCS\n"
#define CONFIG_HLP_STMON         "Enable processing timing measurement of lte softmodem on per subframe basis \n"
#define CONFIG_HLP_256QAM        "Use the 256 QAM mcs table for PDSCH\n"
#define CONFIG_HLP_CHESTFREQ     "Set channel estimation type in frequency domain. 0-Linear interpolation (default). 1-PRB based averaging of channel estimates in frequency. \n"
#define CONFIG_HLP_CHESTTIME     "Set channel estimation type in time domain. 0-Symbols take estimates of the last preceding DMRS symbol (default). 1-Symbol based averaging of channel estimates in time. \n"

#define CONFIG_HLP_NONSTOP       "Go back to frame sync mode after 100 consecutive PBCH failures\n"
//#define CONFIG_HLP_NUMUES        "Set the number of UEs for the emulation"
#define CONFIG_HLP_MSLOTS        "Skip the missed slots/subframes \n"
#define CONFIG_HLP_ULMCS         "Set the maximum uplink MCS\n"

#define CONFIG_HLP_UE            "Set the lte softmodem as a UE\n"
#define CONFIG_HLP_TQFS          "Apply three-quarter of sampling frequency, 23.04 Msps to reduce the data rate on USB/PCIe transfers (only valid for 20 MHz)\n"
#define CONFIG_HLP_TPORT         "tracer port\n"
#define CONFIG_HLP_NOTWAIT       "don't wait for tracer, start immediately\n"
#define CONFIG_HLP_TNOFORK       "to ease debugging with gdb\n"

#define CONFIG_HLP_NUMEROLOGY    "adding numerology for 5G\n"
#define CONFIG_HLP_BAND          "band index\n"
#define CONFIG_HLP_EMULATE_RF    "Emulated RF enabled(disable by defult)\n"
#define CONFIG_HLP_PARALLEL_CMD  "three config for level of parallelism 'PARALLEL_SINGLE_THREAD', 'PARALLEL_RU_L1_SPLIT', or 'PARALLEL_RU_L1_TRX_SPLIT'\n"
#define CONFIG_HLP_WORKER_CMD    "two option for worker 'WORKER_DISABLE' or 'WORKER_ENABLE'\n"
#define CONFIG_HLP_USRP_THREAD   "having extra thead for usrp tx\n"

#define CONFIG_HLP_NOS1          "Disable s1 interface\n"
#define CONFIG_HLP_RFSIM         "Run in rf simulator mode\n"
#define CONFIG_HLP_NOKRNMOD      "(noS1 only): Use tun instead of namesh module \n"
#define CONFIG_HLP_DISABLNBIOT   "disable nb-iot, even if defined in config\n"
#define CONFIG_HLP_USRP_THREAD   "having extra thead for usrp tx\n"
#define CONFIG_HLP_NFAPI         "Change the nFAPI mode for NR 'MONOLITHIC', 'PNF', 'VNF','UE_STUB_PNF','UE_STUB_OFFNET','STANDALONE_PNF'\n"
#define CONFIG_L1_EMULATOR       "Run in L1 emulated mode (disable PHY layer)\n"
#define CONFIG_HLP_CONTINUOUS_TX "perform continuous transmission, even in TDD mode (to work around USRP issues)\n"
#define CONFIG_HLP_STATS_DISABLE "disable globally the stats generation and persistence"
#define CONFIG_HLP_NOITTI        "Do not start itti threads, call queue processing in place, inside the caller thread"
#define CONFIG_HLP_LDPC_OFFLOAD  "Enable LDPC offload to AMD Xilinx T2 telco card\n"
#define CONFIG_HLP_SYNC_REF      "UE acts a Sync Reference in Sidelink. 0-none 1-GNB 2-GNSS 4-localtiming\n"

/*-----------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters common to eNodeB and UE                                                          */
/*   optname                 helpstr                  paramflags      XXXptr                              defXXXval              type         numelt   */
/*-----------------------------------------------------------------------------------------------------------------------------------------------------*/
#define RF_CONFIG_FILE      softmodem_params.rf_config_file
#define TP_CONFIG           softmodem_params.threadPoolConfig
#define CONTINUOUS_TX       softmodem_params.continuous_tx
#define PHY_TEST            softmodem_params.phy_test
#define DO_RA               softmodem_params.do_ra
#define SA                  softmodem_params.sa
#define SL_MODE             softmodem_params.sl_mode
#define WAIT_FOR_SYNC       softmodem_params.wait_for_sync
#define SINGLE_THREAD_FLAG  softmodem_params.single_thread_flag
#define CHAIN_OFFSET        softmodem_params.chain_offset
#define NUMEROLOGY          softmodem_params.numerology
#define BAND                softmodem_params.band
#define EMULATE_RF          softmodem_params.emulate_rf
#define CLOCK_SOURCE        softmodem_params.clock_source
#define TIMING_SOURCE       softmodem_params.timing_source
#define TUNE_OFFSET         softmodem_params.tune_offset
#define SEND_DMRSSYNC       softmodem_params.send_dmrs_sync
#define USIM_TEST           softmodem_params.usim_test
#define CHEST_FREQ          softmodem_params.chest_freq
#define CHEST_TIME          softmodem_params.chest_time
#define NFAPI               softmodem_params.nfapi
#define NSA                 softmodem_params.nsa
#define NODE_NUMBER         softmodem_params.node_number
#define NON_STOP            softmodem_params.non_stop
#define EMULATE_L1          softmodem_params.emulate_l1
#define CONTINUOUS_TX       softmodem_params.continuous_tx
#define SYNC_REF            softmodem_params.sync_ref
#define LDPC_OFFLOAD_FLAG   softmodem_params.ldpc_offload_flag

#define DEFAULT_RFCONFIG_FILE    "/usr/local/etc/syriq/ue.band7.tm1.PRB100.NR40.dat";

extern int usrp_tx_thread;
// clang-format off
#define CMDLINE_PARAMS_DESC {  \
  {"rf-config-file",        CONFIG_HLP_RFCFGF,        0,              .strptr=&RF_CONFIG_FILE,                .defstrval=NULL,          TYPE_STRING, 0},  \
  {"thread-pool",           CONFIG_HLP_TPOOL,         0,              .strptr=&TP_CONFIG,                     .defstrval="-1,-1,-1,-1,-1,-1,-1,-1",  TYPE_STRING, 0},     \
  {"phy-test",              CONFIG_HLP_PHYTST,        PARAMFLAG_BOOL, .iptr=&PHY_TEST,                        .defintval=0,             TYPE_INT,    0},  \
  {"do-ra",                 CONFIG_HLP_DORA,          PARAMFLAG_BOOL, .iptr=&DO_RA,                           .defintval=0,             TYPE_INT,    0},  \
  {"sa",                    CONFIG_HLP_SA,            PARAMFLAG_BOOL, .iptr=&SA,                              .defintval=0,             TYPE_INT,    0},  \
  {"sl-mode",               CONFIG_HLP_SL_MODE,       0,              .u8ptr=&SL_MODE,                        .defintval=0,             TYPE_UINT8,  0},  \
  {"usim-test",             CONFIG_HLP_USIM,          PARAMFLAG_BOOL, .u8ptr=&USIM_TEST,                      .defintval=0,             TYPE_UINT8,  0},  \
  {"clock-source",          CONFIG_HLP_CLK,           0,              .uptr=&CLOCK_SOURCE,                    .defintval=0,             TYPE_UINT,   0},  \
  {"time-source",           CONFIG_HLP_TME,           0,              .uptr=&TIMING_SOURCE,                   .defintval=0,             TYPE_UINT,   0},  \
  {"tune-offset",           CONFIG_HLP_TUNE_OFFSET,   0,              .dblptr=&TUNE_OFFSET,                   .defintval=0,             TYPE_DOUBLE, 0},  \
  {"wait-for-sync",         NULL,                     PARAMFLAG_BOOL, .iptr=&WAIT_FOR_SYNC,                   .defintval=0,             TYPE_INT,    0},  \
  {"single-thread-enable",  CONFIG_HLP_NOSNGLT,       PARAMFLAG_BOOL, .iptr=&SINGLE_THREAD_FLAG,              .defintval=0,             TYPE_INT,    0},  \
  {"C" ,                    CONFIG_HLP_DLF,           0,              .u64ptr=&(downlink_frequency[0][0]),    .defuintval=0,            TYPE_UINT64, 0},  \
  {"CO" ,                   CONFIG_HLP_ULF,           0,              .iptr=&(uplink_frequency_offset[0][0]), .defintval=0,             TYPE_INT,    0},  \
  {"a" ,                    CONFIG_HLP_CHOFF,         0,              .iptr=&CHAIN_OFFSET,                    .defintval=0,             TYPE_INT,    0},  \
  {"d" ,                    CONFIG_HLP_SOFTS,         PARAMFLAG_BOOL, .uptr=&do_forms,                        .defintval=0,             TYPE_UINT,   0},  \
  {"dqt" ,                  CONFIG_HLP_SOFTS_QT,      PARAMFLAG_BOOL, .uptr=&do_forms_qt,                     .defintval=0,             TYPE_UINT,   0},  \
  {"q" ,                    CONFIG_HLP_STMON,         PARAMFLAG_BOOL, .iptr=&opp_enabled,                     .defintval=0,             TYPE_INT,    0},  \
  {"numerology" ,           CONFIG_HLP_NUMEROLOGY,    0,              .iptr=&NUMEROLOGY,                      .defintval=1,             TYPE_INT,    0},  \
  {"band" ,                 CONFIG_HLP_BAND,          0,              .iptr=&BAND,                            .defintval=78,            TYPE_INT,    0},  \
  {"emulate-rf" ,           CONFIG_HLP_EMULATE_RF,    PARAMFLAG_BOOL, .iptr=&EMULATE_RF,                      .defintval=0,             TYPE_INT,    0},  \
  {"parallel-config",       CONFIG_HLP_PARALLEL_CMD,  0,              .strptr=&parallel_config,               .defstrval=NULL,          TYPE_STRING, 0},  \
  {"worker-config",         CONFIG_HLP_WORKER_CMD,    0,              .strptr=&worker_config,                 .defstrval=NULL,          TYPE_STRING, 0},  \
  {"noS1",                  CONFIG_HLP_NOS1,          PARAMFLAG_BOOL, .uptr=&noS1,                            .defintval=0,             TYPE_UINT,   0},  \
  {"rfsim",                 CONFIG_HLP_RFSIM,         PARAMFLAG_BOOL, .uptr=&rfsim,                           .defintval=0,             TYPE_UINT,   0},  \
  {"nokrnmod",              CONFIG_HLP_NOKRNMOD,      PARAMFLAG_BOOL, .uptr=&nokrnmod,                        .defintval=1,             TYPE_UINT,   0},  \
  {"nbiot-disable",         CONFIG_HLP_DISABLNBIOT,   PARAMFLAG_BOOL, .uptr=&nonbiot,                         .defuintval=0,            TYPE_UINT,   0},  \
  {"chest-freq",            CONFIG_HLP_CHESTFREQ,     0,              .iptr=&CHEST_FREQ,                      .defintval=0,             TYPE_INT,    0},  \
  {"chest-time",            CONFIG_HLP_CHESTTIME,     0,              .iptr=&CHEST_TIME,                      .defintval=0,             TYPE_INT,    0},  \
  {"nsa",                   CONFIG_HLP_NSA,           PARAMFLAG_BOOL, .iptr=&NSA,                             .defintval=0,             TYPE_INT,    0},  \
  {"node-number",           NULL,                     0,              .u16ptr=&NODE_NUMBER,                   .defuintval=0,            TYPE_UINT16, 0},  \
  {"usrp-tx-thread-config", CONFIG_HLP_USRP_THREAD,   0,              .iptr=&usrp_tx_thread,                  .defstrval=0,             TYPE_INT,    0},  \
  {"nfapi",                 CONFIG_HLP_NFAPI,         0,              .strptr=NULL,                           .defstrval="MONOLITHIC",  TYPE_STRING, 0},  \
  {"non-stop",              CONFIG_HLP_NONSTOP,       PARAMFLAG_BOOL, .iptr=&NON_STOP,                        .defintval=0,             TYPE_INT,    0},  \
  {"emulate-l1",            CONFIG_L1_EMULATOR,       PARAMFLAG_BOOL, .iptr=&EMULATE_L1,                      .defintval=0,             TYPE_INT,    0},  \
  {"continuous-tx",         CONFIG_HLP_CONTINUOUS_TX, PARAMFLAG_BOOL, .iptr=&CONTINUOUS_TX,                   .defintval=0,             TYPE_INT,    0},  \
  {"disable-stats",         CONFIG_HLP_STATS_DISABLE, PARAMFLAG_BOOL, .iptr=&stats_disabled,                  .defintval=0,             TYPE_INT,    0},  \
  {"no-itti-threads",       CONFIG_HLP_NOITTI,        PARAMFLAG_BOOL, .iptr=&softmodem_params.no_itti,        .defintval=0,             TYPE_INT,    0},  \
  {"ldpc-offload-enable",   CONFIG_HLP_LDPC_OFFLOAD,  PARAMFLAG_BOOL, .iptr=&LDPC_OFFLOAD_FLAG,               .defstrval=0,             TYPE_INT,    0},  \
  {"sync-ref",              CONFIG_HLP_SYNC_REF,      0,              .uptr=&SYNC_REF,                        .defintval=0,             TYPE_UINT,   0},  \
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
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s5 = { NULL } },                     \
    { .s3a = { config_checkstr_assign_integer, \
               {"MONOLITHIC", "PNF", "VNF","UE_STUB_PNF","UE_STUB_OFFNET","STANDALONE_PNF"}, \
               {NFAPI_MONOLITHIC, NFAPI_MODE_PNF, NFAPI_MODE_VNF,NFAPI_UE_STUB_PNF,NFAPI_UE_STUB_OFFNET,NFAPI_MODE_STANDALONE_PNF}, \
               6 } }, \
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

#define SOFTMODEM_NOS1_BIT            (1<<0)
#define SOFTMODEM_NOKRNMOD_BIT        (1<<1)
#define SOFTMODEM_NONBIOT_BIT         (1<<2)
#define SOFTMODEM_RFSIM_BIT           (1<<10)
#define SOFTMODEM_SIML1_BIT           (1<<12)
#define SOFTMODEM_DLSIM_BIT           (1<<13)
#define SOFTMODEM_DOSCOPE_BIT         (1<<15)
#define SOFTMODEM_RECPLAY_BIT         (1<<16)
#define SOFTMODEM_TELNETCLT_BIT       (1<<17)
#define SOFTMODEM_RECRECORD_BIT       (1<<18)
#define SOFTMODEM_ENB_BIT             (1<<20)
#define SOFTMODEM_GNB_BIT             (1<<21)
#define SOFTMODEM_4GUE_BIT            (1<<22)
#define SOFTMODEM_5GUE_BIT            (1<<23)
#define SOFTMODEM_NOSTATS_BIT         (1<<24)
#define SOFTMODEM_DOSCOPE_QT_BIT      (1<<25)

#define SOFTMODEM_FUNC_BITS (SOFTMODEM_ENB_BIT | SOFTMODEM_GNB_BIT | SOFTMODEM_5GUE_BIT | SOFTMODEM_4GUE_BIT)
#define MAPPING_SOFTMODEM_FUNCTIONS {{"enb",SOFTMODEM_ENB_BIT},{"gnb",SOFTMODEM_GNB_BIT},{"4Gue",SOFTMODEM_4GUE_BIT},{"5Gue",SOFTMODEM_5GUE_BIT}}


#define IS_SOFTMODEM_NOS1            ( get_softmodem_optmask() & SOFTMODEM_NOS1_BIT)
#define IS_SOFTMODEM_NOKRNMOD        ( get_softmodem_optmask() & SOFTMODEM_NOKRNMOD_BIT)
#define IS_SOFTMODEM_NONBIOT         ( get_softmodem_optmask() & SOFTMODEM_NONBIOT_BIT)
#define IS_SOFTMODEM_RFSIM           ( get_softmodem_optmask() & SOFTMODEM_RFSIM_BIT)
#define IS_SOFTMODEM_SIML1           ( get_softmodem_optmask() & SOFTMODEM_SIML1_BIT)
#define IS_SOFTMODEM_DLSIM           ( get_softmodem_optmask() & SOFTMODEM_DLSIM_BIT)
#define IS_SOFTMODEM_DOSCOPE         ( get_softmodem_optmask() & SOFTMODEM_DOSCOPE_BIT)
#define IS_SOFTMODEM_DOSCOPE_QT      ( get_softmodem_optmask() & SOFTMODEM_DOSCOPE_QT_BIT)
#define IS_SOFTMODEM_IQPLAYER        ( get_softmodem_optmask() & SOFTMODEM_RECPLAY_BIT)
#define IS_SOFTMODEM_IQRECORDER      ( get_softmodem_optmask() & SOFTMODEM_RECRECORD_BIT)
#define IS_SOFTMODEM_TELNETCLT_BIT   ( get_softmodem_optmask() & SOFTMODEM_TELNETCLT_BIT)    
#define IS_SOFTMODEM_ENB_BIT         ( get_softmodem_optmask() & SOFTMODEM_ENB_BIT)
#define IS_SOFTMODEM_GNB_BIT         ( get_softmodem_optmask() & SOFTMODEM_GNB_BIT)
#define IS_SOFTMODEM_4GUE_BIT        ( get_softmodem_optmask() & SOFTMODEM_4GUE_BIT)
#define IS_SOFTMODEM_5GUE_BIT        ( get_softmodem_optmask() & SOFTMODEM_5GUE_BIT)
#define IS_SOFTMODEM_NOSTATS_BIT     ( get_softmodem_optmask() & SOFTMODEM_NOSTATS_BIT)

typedef struct {
  uint64_t       optmask;
  //THREAD_STRUCT  thread_struct;
  char           *rf_config_file;
  char *threadPoolConfig;
  int            phy_test;
  int            do_ra;
  int            sa;
  uint8_t        sl_mode;
  uint8_t        usim_test;
  int            emulate_rf;
  int            wait_for_sync; //eNodeB only
  int            single_thread_flag; //eNodeB only
  int            chain_offset;
  int            numerology;
  int            band;
  uint32_t       clock_source;
  uint32_t       timing_source;
  double         tune_offset;
  int            hw_timing_advance;
  uint32_t       send_dmrs_sync;
  int            use_256qam_table;
  int            chest_time;
  int            chest_freq;
  uint8_t        nfapi;
  int            nsa;
  uint16_t       node_number;
  int            non_stop;
  int            emulate_l1;
  int            continuous_tx;
  uint32_t       sync_ref;
  int no_itti;
  int ldpc_offload_flag;
} softmodem_params_t;

extern uint64_t get_softmodem_optmask(void);
extern uint64_t set_softmodem_optmask(uint64_t bitmask);
extern uint64_t clear_softmodem_optmask(uint64_t bitmask);
extern softmodem_params_t *get_softmodem_params(void);
extern void get_common_options(configmodule_interface_t *cfg, uint32_t execmask);
extern char *get_softmodem_function(uint64_t *sofmodemfunc_mask_ptr);
#define SOFTMODEM_RTSIGNAL  (SIGRTMIN+1)
extern void set_softmodem_sighandler(void);
extern uint64_t downlink_frequency[MAX_NUM_CCs][4];
extern int32_t uplink_frequency_offset[MAX_NUM_CCs][4];
extern int usrp_tx_thread;
extern uint16_t sl_ahead;
extern uint16_t sf_ahead;
extern int oai_exit;

void rx_func(void *param);
void ru_tx_func(void *param);
extern uint8_t nfapi_mode;
extern int16_t vnf_pnf_sfnslot_delta;
#ifdef __cplusplus
}
#endif
#endif
