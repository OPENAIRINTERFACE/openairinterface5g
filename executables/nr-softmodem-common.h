#ifndef NR_SOFTMODEM_COMMON_H
#define NR_SOFTMODEM_COMMON_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <execinfo.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/sched.h>
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
#include "targets/ARCH/COMMON/common_lib.h"
#undef MALLOC
#include "assertions.h"
#include "PHY/types.h"
#include <threadPool/thread-pool.h>

#if defined(ENABLE_USE_MME)
#include "s1ap_eNB.h"
#ifdef PDCP_USE_NETLINK
#include "SIMULATION/ETH_TRANSPORT/proto.h"
#endif
#endif

/* help strings definition for command line options, used in CMDLINE_XXX_DESC macros and printed when -h option is used */
#define CONFIG_HLP_RFCFGF        "Configuration file for front-end (e.g. LMS7002M)\n"
#define CONFIG_HLP_ULMAXE        "set the eNodeB max ULSCH erros\n"
#define CONFIG_HLP_CALUER        "set UE RX calibration\n"
#define CONFIG_HLP_CALUERM       ""
#define CONFIG_HLP_CALUERB       ""
#define CONFIG_HLP_DBGUEPR       "UE run normal prach power ramping, but don't continue random-access\n"
#define CONFIG_HLP_CALPRACH      "UE run normal prach with maximum power, but don't continue random-access\n"
#define CONFIG_HLP_NOL2CN        "bypass L2 and upper layers\n"
#define CONFIG_HLP_UERXG         "set UE RX gain\n"
#define CONFIG_HLP_UERXGOFF      "external UE amplifier offset\n"
#define CONFIG_HLP_UETXG         "set UE TX gain\n"
#define CONFIG_HLP_UENANTR       "set UE number of rx antennas\n"
#define CONFIG_HLP_UENANTT       "set UE number of tx antennas\n"
#define CONFIG_HLP_UESCAN        "set UE to scan around carrier\n"
#define CONFIG_HLP_UEFO          "set UE to enable estimation and compensation of frequency offset\n"
#define CONFIG_HLP_DUMPFRAME     "dump UE received frame to rxsig_frame0.dat and exit\n"
#define CONFIG_HLP_DLSHIFT       "dynamic shift for LLR compuation for TM3/4 (default 0)\n"
#define CONFIG_HLP_UELOOP        "get softmodem (UE) to loop through memory instead of acquiring from HW\n"
#define CONFIG_HLP_PHYTST        "test UE phy layer, mac disabled\n"
#define CONFIG_HLP_DORA          "test gNB  and UE with RA procedures\n"
#define CONFIG_HLP_DMAMAP        "sets flag for improved EXMIMO UE performance\n"
#define CONFIG_HLP_EXCCLK        "tells hardware to use a clock reference (0:internal(default), 1:external, 2:gpsdo)\n"
#define CONFIG_HLP_USIM          "use XOR autentication algo in case of test usim mode\n"
#define CONFIG_HLP_NOSNGLT       "Disables single-thread mode in lte-softmodem\n"
#define CONFIG_HLP_TADV          "Set timing_advance\n"
#define CONFIG_HLP_DLF           "Set the downlink frequency for all component carriers\n"
#define CONFIG_HLP_CHOFF         "Channel id offset\n"
#define CONFIG_HLP_SOFTS         "Enable soft scope and L1 and L2 stats (Xforms)\n"
#define CONFIG_HLP_EXMCAL        "Calibrate the EXMIMO borad, available files: exmimo2_2arxg.lime exmimo2_2brxg.lime \n"
#define CONFIG_HLP_ITTIL         "Generate ITTI analyzser logs (similar to wireshark logs but with more details)\n"
#define CONFIG_HLP_DLMCS_PHYTEST "Set the downlink MCS for PHYTEST mode\n"
#define CONFIG_HLP_STMON         "Enable processing timing measurement of lte softmodem on per subframe basis \n"
#define CONFIG_HLP_PRB           "Set the PRB, valid values: 6, 25, 50, 100  \n"
#define CONFIG_HLP_MSLOTS        "Skip the missed slots/subframes \n"
#define CONFIG_HLP_ULMCS_PHYTEST "Set the uplink MCS for PHYTEST mode\n"
#define CONFIG_HLP_DLBW_PHYTEST  "Set the number of PRBs used for DLSCH in PHYTEST mode\n"
#define CONFIG_HLP_ULBW_PHYTEST  "Set the number of PRBs used for ULSCH in PHYTEST mode\n"
#define CONFIG_HLP_DLBM_PHYTEST  "Bitmap for DLSCH slots (slot 0 starts at LSB)\n"
#define CONFIG_HLP_ULBM_PHYTEST  "Bitmap for ULSCH slots (slot 0 starts at LSB)\n"
#define CONFIG_HLP_TDD           "Set hardware to TDD mode (default: FDD). Used only with -U (otherwise set in config file).\n"
#define CONFIG_HLP_UE            "Set the lte softmodem as a UE\n"
#define CONFIG_HLP_L2MONW        "Enable L2 wireshark messages on localhost \n"
#define CONFIG_HLP_L2MONP        "Enable L2 pcap  messages on localhost \n"
#define CONFIG_HLP_MAC           "Disable the MAC procedures at UE side (default is enabled)\n"
#define CONFIG_HLP_VCD           "Enable VCD (generated file will is named openair_dump_eNB.vcd, read it with target/RT/USER/eNB.gtkw\n"
#define CONFIG_HLP_TQFS          "Apply three-quarter of sampling frequency, 23.04 Msps to reduce the data rate on USB/PCIe transfers (only valid for 20 MHz)\n"
#define CONFIG_HLP_TPORT         "tracer port\n"
#define CONFIG_HLP_NOTWAIT       "don't wait for tracer, start immediately\n"
#define CONFIG_HLP_TNOFORK       "to ease debugging with gdb\n"
#define CONFIG_HLP_DISABLNBIOT   "disable nb-iot, even if defined in config\n"
#define CONFIG_HLP_DISABLETIMECORR   "disable UE timing correction\n"
#define CONFIG_HLP_RRC_CFG_PATH   "path for RRC configuration\n"

#define CONFIG_HLP_NUMEROLOGY    "adding numerology for 5G\n"
#define CONFIG_HLP_EMULATE_RF    "Emulated RF enabled(disable by defult)\n"
#define CONFIG_HLP_PARALLEL_CMD  "three config for level of parallelism 'PARALLEL_SINGLE_THREAD', 'PARALLEL_RU_L1_SPLIT', or 'PARALLEL_RU_L1_TRX_SPLIT'\n"
#define CONFIG_HLP_WORKER_CMD    "two option for worker 'WORKER_DISABLE' or 'WORKER_ENABLE'\n"
#define CONFIG_HLP_USRP_THREAD   "having extra thead for usrp tx\n"
#define CONFIG_HLP_DISABLNBIOT   "disable nb-iot, even if defined in config\n"

#define CONFIG_HLP_USRP_ARGS     "set the arguments to identify USRP (same syntax as in UHD)\n"

#define CONFIG_HLP_FLOG          "Enable online log \n"
#define CONFIG_HLP_LOGL          "Set the global log level, valid options: (4:trace, 3:debug, 2:info, 1:warn, (0:error))\n"
#define CONFIG_HLP_LOGV          "Set the global log verbosity \n"
#define CONFIG_HLP_TELN          "Start embedded telnet server \n"
#define CONFIG_HLP_MSC           "Enable the MSC tracing utility \n"
#define CONFIG_HLP_SNR           "Set average SNR in dB (for --siml1 option)\n"
#define CONFIG_HLP_NOS1          "Disable s1 interface\n"
#define CONFIG_HLP_NOKRNMOD      "(noS1 only): Use tun instead of namesh module \n"
/*--------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters for LOG utility                                             */
/*   optname         helpstr          paramflags          XXXptr                     defXXXval            type           numelt   */
/*--------------------------------------------------------------------------------------------------------------------------------*/
#define START_MSC                softmodem_params.start_msc
#define CMDLINE_LOGPARAMS_DESC_NR {  \
    {"R" ,           CONFIG_HLP_FLOG, 0,                  uptr:&online_log_messages, defintval:1,         TYPE_INT,      0},       \
    {"g" ,           CONFIG_HLP_LOGL, 0,                  uptr:&glog_level,          defintval:0,         TYPE_UINT,     0},       \
    {"G" ,           CONFIG_HLP_LOGV, 0,                  uptr:&glog_verbosity,      defintval:0,         TYPE_UINT16,   0},       \
    {"telnetsrv",    CONFIG_HLP_TELN, PARAMFLAG_BOOL,     uptr:&start_telnetsrv,     defintval:0,         TYPE_UINT,     0},       \
	{"msc",          CONFIG_HLP_MSC,  PARAMFLAG_BOOL,     uptr:&START_MSC,           defintval:0,         TYPE_UINT,     0},       \
  }

#define CMDLINE_ONLINELOG_IDX     0
#define CMDLINE_GLOGLEVEL_IDX     1
#define CMDLINE_GLOGVERBO_IDX     2
#define CMDLINE_STARTTELN_IDX     3

/***************************************************************************************************************************************/


extern pthread_cond_t sync_cond;
extern pthread_mutex_t sync_mutex;
extern int sync_var;


extern uint64_t downlink_frequency[MAX_NUM_CCs][4];
extern int32_t uplink_frequency_offset[MAX_NUM_CCs][4];

extern int rx_input_level_dBm;
extern uint64_t num_missed_slots; // counter for the number of missed slots

extern int oaisim_flag;
extern volatile int  oai_exit;

extern openair0_config_t openair0_cfg[MAX_CARDS];
extern pthread_cond_t sync_cond;
extern pthread_mutex_t sync_mutex;
extern int sync_var;
extern int transmission_mode;
extern double cpuf;

extern int emulate_rf;
extern int numerology;
extern int usrp_tx_thread;

#endif
