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

/*! \file lte-softmodem-common.h
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
#define CONFIG_HLP_SPLIT73       "Split 7.3 (below rate matching) option: <cu|du>:<remote ip address>:<remote port>"
#define CONFIG_HLP_TPOOL         "Thread pool configuration: \n\
  default no pool (runs in calling thread),\n\
  list of cores, comma separated (negative value is no core affinity)\n\
  example: -1,3 launches two working threads one floating, the second set on core 3"
#define CONFIG_HLP_ULMAXE        "set the eNodeB max ULSCH erros\n"
#define CONFIG_HLP_CALUER        "set UE RX calibration\n"
#define CONFIG_HLP_CALUERM       ""
#define CONFIG_HLP_CALUERB       ""
#define CONFIG_HLP_DBGUEPR       "UE run normal prach power ramping, but don't continue random-access\n"
#define CONFIG_HLP_CALPRACH      "UE run normal prach with maximum power, but don't continue random-access\n"
#define CONFIG_HLP_NOL2CN        "bypass L2 and upper layers\n"


#define CONFIG_HLP_DUMPFRAME     "dump UE received frame to rxsig_frame0.dat and exit\n"
#define CONFIG_HLP_UELOOP        "get softmodem (UE) to loop through memory instead of acquiring from HW\n"
#define CONFIG_HLP_PHYTST        "test UE phy layer, mac disabled\n"
#define CONFIG_HLP_DORA          "test gNB  and UE with RA procedures\n"
#define CONFIG_HLP_SA            "run gNB in standalone mode\n"
#define CONFIG_HLP_EXTS          "tells hardware to use an external timing reference\n"
#define CONFIG_HLP_DMRSSYNC      "tells RU to insert DMRS in subframe 1 slot 0"
#define CONFIG_HLP_CLK           "tells hardware to use a clock reference (0:internal, 1:external, 2:gpsdo)\n"
#define CONFIG_HLP_TME           "tells hardware to use a time reference (0:internal, 1:external, 2:gpsdo)\n"
#define CONFIG_HLP_USIM          "use XOR autentication algo in case of test usim mode\n"
#define CONFIG_HLP_NOSNGLT       "Disables single-thread mode in lte-softmodem\n"
#define CONFIG_HLP_DLF           "Set the downlink frequency for all component carriers\n"
#define CONFIG_HLP_ULF           "Set the uplink frequency offset for all component carriers\n"
#define CONFIG_HLP_CHOFF         "Channel id offset\n"
#define CONFIG_HLP_SOFTS         "Enable soft scope and L1 and L2 stats (Xforms)\n"
#define CONFIG_HLP_EXMCAL        "Calibrate the EXMIMO borad, available files: exmimo2_2arxg.lime exmimo2_2brxg.lime \n"
#define CONFIG_HLP_ITTIL         "Generate ITTI analyzser logs (similar to wireshark logs but with more details)\n"
#define CONFIG_HLP_DLMCS         "Set the maximum downlink MCS\n"
#define CONFIG_HLP_STMON         "Enable processing timing measurement of lte softmodem on per subframe basis \n"
#define CONFIG_HLP_256QAM        "Use the 256 QAM mcs table for PDSCH\n"
#define CONFIG_HLP_PRBINTER       "Do PRB based averaging of channel estimates. Frequency domain linear interpolation by default\n"

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
#define CONFIG_HLP_RFSIM         "Run in rf simulator mode (also known as basic simulator)\n"
#define CONFIG_HLP_NOKRNMOD      "(noS1 only): Use tun instead of namesh module \n"
#define CONFIG_HLP_DISABLNBIOT   "disable nb-iot, even if defined in config\n"
#define CONFIG_HLP_USRP_THREAD   "having extra thead for usrp tx\n"
#define CONFIG_HLP_NFAPI         "Change the nFAPI mode for NR\n"

/*-----------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters common to eNodeB and UE                                                          */
/*   optname                 helpstr                  paramflags      XXXptr                              defXXXval              type         numelt   */
/*-----------------------------------------------------------------------------------------------------------------------------------------------------*/
#define RF_CONFIG_FILE      softmodem_params.rf_config_file
#define SPLIT73             softmodem_params.split73
#define TP_CONFIG           softmodem_params.threadPoolConfig
#define PHY_TEST            softmodem_params.phy_test
#define DO_RA               softmodem_params.do_ra
#define SA                  softmodem_params.sa
#define WAIT_FOR_SYNC       softmodem_params.wait_for_sync
#define SINGLE_THREAD_FLAG  softmodem_params.single_thread_flag
#define CHAIN_OFFSET        softmodem_params.chain_offset
#define NUMEROLOGY          softmodem_params.numerology
#define BAND                softmodem_params.band
#define EMULATE_RF          softmodem_params.emulate_rf
#define CLOCK_SOURCE        softmodem_params.clock_source
#define TIMING_SOURCE       softmodem_params.timing_source
#define SEND_DMRSSYNC       softmodem_params.send_dmrs_sync
#define USIM_TEST           softmodem_params.usim_test
#define USE_256QAM_TABLE    softmodem_params.use_256qam_table
#define PRB_INTERPOLATION   softmodem_params.prb_interpolation
#define NFAPI               softmodem_params.nfapi
#define NON_STOP            softmodem_params.non_stop

#define DEFAULT_RFCONFIG_FILE    "/usr/local/etc/syriq/ue.band7.tm1.PRB100.NR40.dat";

extern int usrp_tx_thread;
#define CMDLINE_PARAMS_DESC {  \
    {"rf-config-file",       CONFIG_HLP_RFCFGF,       0,              strptr:(char **)&RF_CONFIG_FILE,    defstrval:NULL,        TYPE_STRING, sizeof(RF_CONFIG_FILE)},\
    {"split73",              CONFIG_HLP_SPLIT73,      0,              strptr:(char **)&SPLIT73,           defstrval:NULL,        TYPE_STRING, sizeof(SPLIT73)},       \
    {"thread-pool",          CONFIG_HLP_TPOOL,        0,              strptr:(char **)&TP_CONFIG,         defstrval:"n",         TYPE_STRING, sizeof(TP_CONFIG)},     \
    {"phy-test",             CONFIG_HLP_PHYTST,       PARAMFLAG_BOOL, iptr:&PHY_TEST,                     defintval:0,           TYPE_INT,    0},                     \
    {"do-ra",                CONFIG_HLP_DORA,         PARAMFLAG_BOOL, iptr:&DO_RA,                        defintval:0,           TYPE_INT,    0},                     \
    {"sa",                   CONFIG_HLP_SA,           PARAMFLAG_BOOL, iptr:&SA,                           defintval:0,           TYPE_INT,    0},                     \
    {"usim-test",            CONFIG_HLP_USIM,         PARAMFLAG_BOOL, u8ptr:&USIM_TEST,                   defintval:0,           TYPE_UINT8,  0},                     \
    {"clock-source",         CONFIG_HLP_CLK,          0,              uptr:&CLOCK_SOURCE,                 defintval:0,           TYPE_UINT,   0},                     \
    {"time-source",          CONFIG_HLP_TME,          0,              uptr:&TIMING_SOURCE,                defintval:0,           TYPE_UINT,   0},                     \
    {"wait-for-sync",        NULL,                    PARAMFLAG_BOOL, iptr:&WAIT_FOR_SYNC,                defintval:0,           TYPE_INT,    0},                     \
    {"single-thread-enable", CONFIG_HLP_NOSNGLT,      PARAMFLAG_BOOL, iptr:&SINGLE_THREAD_FLAG,           defintval:0,           TYPE_INT,    0},                     \
    {"C" ,                   CONFIG_HLP_DLF,          0,              u64ptr:&(downlink_frequency[0][0]), defuintval:0,          TYPE_UINT64, 0},                     \
    {"CO" ,                  CONFIG_HLP_ULF,          0,              iptr:&(uplink_frequency_offset[0][0]), defintval:0,        TYPE_INT,    0},                     \
    {"a" ,                   CONFIG_HLP_CHOFF,        0,              iptr:&CHAIN_OFFSET,                 defintval:0,           TYPE_INT,    0},                     \
    {"d" ,                   CONFIG_HLP_SOFTS,        PARAMFLAG_BOOL, uptr:(uint32_t *)&do_forms,         defintval:0,           TYPE_INT8,   0},                     \
    {"q" ,                   CONFIG_HLP_STMON,        PARAMFLAG_BOOL, iptr:&opp_enabled,                  defintval:0,           TYPE_INT,    0},                     \
    {"numerology" ,          CONFIG_HLP_NUMEROLOGY,   PARAMFLAG_BOOL, iptr:&NUMEROLOGY,                   defintval:1,           TYPE_INT,    0},                     \
    {"band" ,                CONFIG_HLP_BAND,         PARAMFLAG_BOOL, iptr:&BAND,                         defintval:78,          TYPE_INT,    0},                     \
    {"emulate-rf" ,          CONFIG_HLP_EMULATE_RF,   PARAMFLAG_BOOL, iptr:&EMULATE_RF,                   defintval:0,           TYPE_INT,    0},                     \
    {"parallel-config",      CONFIG_HLP_PARALLEL_CMD, 0,              strptr:(char **)&parallel_config,   defstrval:NULL,        TYPE_STRING, 0},                     \
    {"worker-config",        CONFIG_HLP_WORKER_CMD,   0,              strptr:(char **)&worker_config,     defstrval:NULL,        TYPE_STRING, 0},                     \
    {"noS1",                 CONFIG_HLP_NOS1,         PARAMFLAG_BOOL, uptr:&noS1,                         defintval:0,           TYPE_INT,    0},                     \
    {"rfsim",                CONFIG_HLP_RFSIM,        PARAMFLAG_BOOL, uptr:&rfsim,                        defintval:0,           TYPE_INT,    0},                     \
    {"basicsim",             CONFIG_HLP_RFSIM,        PARAMFLAG_BOOL, uptr:&basicsim,                     defintval:0,           TYPE_INT,    0},                     \
    {"nokrnmod",             CONFIG_HLP_NOKRNMOD,     PARAMFLAG_BOOL, uptr:&nokrnmod,                     defintval:0,           TYPE_INT,    0},                     \
    {"nbiot-disable",        CONFIG_HLP_DISABLNBIOT,  PARAMFLAG_BOOL, uptr:&nonbiot,                      defuintval:0,          TYPE_INT,    0},                     \
    {"use-256qam-table",     CONFIG_HLP_256QAM,       PARAMFLAG_BOOL, iptr:&USE_256QAM_TABLE,             defintval:0,           TYPE_INT,    0},                     \
    {"do-prb-interpolation",  CONFIG_HLP_PRBINTER,     PARAMFLAG_BOOL, iptr:&PRB_INTERPOLATION,            defintval:0,           TYPE_INT,    0},                     \
    {"usrp-tx-thread-config", CONFIG_HLP_USRP_THREAD, 0,              iptr:&usrp_tx_thread,               defstrval:0,           TYPE_INT,    0},        \
    {"nfapi",                CONFIG_HLP_NFAPI,        0,              u8ptr:&nfapi_mode,                       defintval:0,           TYPE_UINT8,  0},                     \
    {"non-stop",            CONFIG_HLP_NONSTOP,      PARAMFLAG_BOOL, iptr:&NON_STOP,                       defintval:0,           TYPE_INT,  0},                     \
  }

  
#define CONFIG_HLP_FLOG          "Enable online log \n"
#define CONFIG_HLP_LOGL          "Set the global log level, valid options: (4:trace, 3:debug, 2:info, 1:warn, (0:error))\n"
#define CONFIG_HLP_LOGV          "Set the global log verbosity \n"
#define CONFIG_HLP_TELN          "Start embedded telnet server \n"
#define CONFIG_HLP_MSC           "Enable the MSC tracing utility \n"
#define CONFIG_FLOG_OPT          "R"
#define CONFIG_LOGL_OPT          "g"
/*-------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters for LOG utility                                                              */
/*   optname                        helpstr       paramflags        XXXptr                              defXXXval            type           numelt */
/*-------------------------------------------------------------------------------------------------------------------------------------------------*/
#define START_MSC                softmodem_params.start_msc
#define CMDLINE_LOGPARAMS_DESC {  \
    {CONFIG_FLOG_OPT ,           CONFIG_HLP_FLOG, 0,                uptr:&online_log_messages,           defintval:1,         TYPE_INT,      0},     \
    {CONFIG_LOGL_OPT ,           CONFIG_HLP_LOGL, 0,                uptr:&glog_level,                    defintval:0,         TYPE_UINT,     0},     \
	{"telnetsrv",                CONFIG_HLP_TELN, PARAMFLAG_BOOL,   uptr:&start_telnetsrv,               defintval:0,         TYPE_UINT,     0},     \
    {"msc",                      CONFIG_HLP_MSC,  PARAMFLAG_BOOL,   uptr:&START_MSC,                     defintval:0,         TYPE_UINT,     0},     \
	{"log-mem",                  NULL,            0,                strptr:(char **)&logmem_filename,    defstrval:NULL,      TYPE_STRING,   0},     \
	{"telnetclt",                NULL,            0,                uptr:&start_telnetclt,               defstrval:NULL,      TYPE_UINT,     0},     \
  }


/* check function for global log level */
#define CMDLINE_LOGPARAMS_CHECK_DESC { \
    { .s5= {NULL} } ,                       \
    { .s2= {config_check_intrange, {0,4}}}, \
    { .s5= {NULL} } ,                       \
    { .s5= {NULL} } ,                       \
    { .s5= {NULL} } ,                       \
    { .s5= {NULL} } ,                       \
  }

/***************************************************************************************************************************************/

#define SOFTMODEM_NOS1_BIT            (1<<0)
#define SOFTMODEM_NOKRNMOD_BIT        (1<<1)
#define SOFTMODEM_NONBIOT_BIT         (1<<2)
#define SOFTMODEM_RFSIM_BIT           (1<<10)
#define SOFTMODEM_BASICSIM_BIT        (1<<11)
#define SOFTMODEM_SIML1_BIT           (1<<12)
#define SOFTMODEM_DOSCOPE_BIT         (1<<15)
#define SOFTMODEM_RECPLAY_BIT         (1<<16)
#define SOFTMODEM_TELNETCLT_BIT       (1<<17)
#define SOFTMODEM_ENB_BIT             (1<<20)
#define SOFTMODEM_GNB_BIT             (1<<21)
#define SOFTMODEM_4GUE_BIT            (1<<22)
#define SOFTMODEM_5GUE_BIT            (1<<23)
#define SOFTMODEM_FUNC_BITS (SOFTMODEM_ENB_BIT | SOFTMODEM_GNB_BIT | SOFTMODEM_5GUE_BIT | SOFTMODEM_4GUE_BIT)
#define MAPPING_SOFTMODEM_FUNCTIONS {{"enb",SOFTMODEM_ENB_BIT},{"gnb",SOFTMODEM_GNB_BIT},{"4Gue",SOFTMODEM_4GUE_BIT},{"5Gue",SOFTMODEM_5GUE_BIT}}


#define IS_SOFTMODEM_NOS1            ( get_softmodem_optmask() & SOFTMODEM_NOS1_BIT)
#define IS_SOFTMODEM_NOKRNMOD        ( get_softmodem_optmask() & SOFTMODEM_NOKRNMOD_BIT)
#define IS_SOFTMODEM_NONBIOT         ( get_softmodem_optmask() & SOFTMODEM_NONBIOT_BIT)
#define IS_SOFTMODEM_RFSIM           ( get_softmodem_optmask() & SOFTMODEM_RFSIM_BIT)
#define IS_SOFTMODEM_BASICSIM        ( get_softmodem_optmask() & SOFTMODEM_BASICSIM_BIT)
#define IS_SOFTMODEM_SIML1           ( get_softmodem_optmask() & SOFTMODEM_SIML1_BIT)
#define IS_SOFTMODEM_DOSCOPE         ( get_softmodem_optmask() & SOFTMODEM_DOSCOPE_BIT)
#define IS_SOFTMODEM_IQPLAYER        ( get_softmodem_optmask() & SOFTMODEM_RECPLAY_BIT)
#define IS_SOFTMODEM_TELNETCLT_BIT   ( get_softmodem_optmask() & SOFTMODEM_TELNETCLT_BIT)    
#define IS_SOFTMODEM_ENB_BIT         ( get_softmodem_optmask() & SOFTMODEM_ENB_BIT)
#define IS_SOFTMODEM_GNB_BIT         ( get_softmodem_optmask() & SOFTMODEM_GNB_BIT)
#define IS_SOFTMODEM_4GUE_BIT        ( get_softmodem_optmask() & SOFTMODEM_4GUE_BIT)
#define IS_SOFTMODEM_5GUE_BIT        ( get_softmodem_optmask() & SOFTMODEM_5GUE_BIT)

typedef struct {
  uint64_t       optmask;
  //THREAD_STRUCT  thread_struct;
  char           rf_config_file[1024];
  char           split73[1024];
  char           threadPoolConfig[1024];
  int            phy_test;
  int            do_ra;
  int            sa;
  uint8_t        usim_test;
  int            emulate_rf;
  int            wait_for_sync; //eNodeB only
  int            single_thread_flag; //eNodeB only
  int            chain_offset;
  int            numerology;
  int            band;
  unsigned int   start_msc;
  uint32_t       clock_source;
  uint32_t       timing_source;
  int            hw_timing_advance;
  uint32_t       send_dmrs_sync;
  int            use_256qam_table;
  int            prb_interpolation;
  uint8_t        nfapi;
  int            non_stop;
} softmodem_params_t;

extern uint64_t get_softmodem_optmask(void);
extern uint64_t set_softmodem_optmask(uint64_t bitmask);
extern softmodem_params_t *get_softmodem_params(void);
extern void get_common_options(uint32_t execmask);
extern char *get_softmodem_function(uint64_t *sofmodemfunc_mask_ptr);
#define SOFTMODEM_RTSIGNAL  (SIGRTMIN+1)
extern void set_softmodem_sighandler(void);
extern uint64_t downlink_frequency[MAX_NUM_CCs][4];
extern int32_t uplink_frequency_offset[MAX_NUM_CCs][4];
extern int usrp_tx_thread;
extern uint16_t sl_ahead;
extern uint16_t sf_ahead;
extern volatile int  oai_exit;

void tx_func(void *param);
void rx_func(void *param);
void ru_tx_func(void *param);
extern uint8_t nfapi_mode;
#ifdef __cplusplus
}
#endif
#endif
