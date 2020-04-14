#ifndef NR_UESOFTMODEM_H
#define NR_UESOFTMODEM_H
#include <executables/nr-softmodem-common.h>
#include <executables/softmodem-common.h>
#include "PHY/defs_nr_UE.h"
#include "SIMULATION/ETH_TRANSPORT/proto.h"
#include <openair2/LAYER2/NR_MAC_gNB/mac_proto.h>

/***************************************************************************************************************************************/
/* command line options definitions, CMDLINE_XXXX_DESC macros are used to initialize paramdef_t arrays which are then used as argument
   when calling config_get or config_getlist functions                                                                                 */


/*------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters defining UE running mode                                              */
/*   optname                     helpstr                paramflags                      XXXptr        defXXXval         type       numelt   */
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


/*---------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters specific to UE                                                                       */
/*   optname                     helpstr             paramflags                  XXXptr                      defXXXval              type          numelt   */
/*---------------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CMDLINE_NRUEPARAMS_DESC {  \
    {"ue-rxgain",                CONFIG_HLP_UERXG,       0,              dblptr:&(rx_gain[0][0]),            defdblval:0,           TYPE_DOUBLE,   0},     \
    {"ue-rxgain-off",            CONFIG_HLP_UERXGOFF,    0,              dblptr:&rx_gain_off,                defdblval:0,           TYPE_DOUBLE,   0},     \
    {"ue-txgain",                CONFIG_HLP_UETXG,       0,              dblptr:&(tx_gain[0][0]),            defdblval:0,           TYPE_DOUBLE,   0},     \
    {"ue-nb-ant-rx",             CONFIG_HLP_UENANTR,     0,              u8ptr:&nb_antenna_rx,               defuintval:1,          TYPE_UINT8,    0},     \
    {"ue-nb-ant-tx",             CONFIG_HLP_UENANTT,     0,              u8ptr:&nb_antenna_tx,               defuintval:1,          TYPE_UINT8,    0},     \
    {"ue-scan-carrier",          CONFIG_HLP_UESCAN,      PARAMFLAG_BOOL, iptr:&UE_scan_carrier,              defintval:0,           TYPE_INT,      0},     \
    {"ue-fo-compensation",       CONFIG_HLP_UEFO,        PARAMFLAG_BOOL, iptr:&UE_fo_compensation,           defintval:0,           TYPE_INT,      0},     \
    {"ue-max-power",             NULL,                   0,              iptr:&(tx_max_power[0]),            defintval:90,          TYPE_INT,      0},     \
    {"r"  ,                      CONFIG_HLP_PRB,         0,              iptr:&(frame_parms[0]->N_RB_DL),    defintval:25,          TYPE_UINT,     0},     \
    {"dlsch-demod-shift",        CONFIG_HLP_DLSHIFT,     0,              iptr:(int32_t *)&dlsch_demod_shift, defintval:0,           TYPE_INT,      0},     \
    {"usrp-args",                CONFIG_HLP_USRP_ARGS,   0,              strptr:(char **)&usrp_args,         defstrval:"type=b200", TYPE_STRING,   0}      \
  }

/*----------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters common to eNodeB and UE                                                         */
/*   optname                helpstr                 paramflags      XXXptr                                 defXXXval              type         numelt */
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/
#define CMDLINE_PARAMS_DESC_UE {  \
  {"single-thread-disable", CONFIG_HLP_NOSNGLT,     PARAMFLAG_BOOL, iptr:&single_thread_flag,              defintval:1,           TYPE_INT,    0}, \
  {"nr-dlsch-demod-shift",  CONFIG_HLP_DLSHIFT,     0,              iptr:(int32_t *)&nr_dlsch_demod_shift, defintval:0,           TYPE_INT,    0}, \
  {"A" ,                    CONFIG_HLP_TADV,        0,              uptr:&timing_advance,                  defintval:0,           TYPE_UINT,   0}, \
  {"C" ,                    CONFIG_HLP_DLF,         0,              u64ptr:&(downlink_frequency[0][0]),    defuintval:0,TYPE_UINT64, 0}, \
  {"a" ,                    CONFIG_HLP_CHOFF,       0,              iptr:&chain_offset,                    defintval:0,           TYPE_INT,    0}, \
  {"d" ,                    CONFIG_HLP_SOFTS,       PARAMFLAG_BOOL, uptr:&do_forms,                        defintval:0,           TYPE_INT,    0}, \
  {"E" ,                    CONFIG_HLP_TQFS,        PARAMFLAG_BOOL, iptr:&threequarter_fs,                 defintval:0,           TYPE_INT,    0}, \
  {"m" ,                    CONFIG_HLP_DLMCS,       0,              uptr:&target_dl_mcs,                   defintval:0,           TYPE_UINT,   0}, \
  {"t" ,                    CONFIG_HLP_ULMCS,       0,              uptr:&target_ul_mcs,                   defintval:0,           TYPE_UINT,   0}, \
  {"q" ,                    CONFIG_HLP_STMON,       PARAMFLAG_BOOL, iptr:&opp_enabled,                     defintval:0,           TYPE_INT,    0}, \
  {"T" ,                    CONFIG_HLP_TDD,         PARAMFLAG_BOOL, iptr:&tddflag,                         defintval:0,           TYPE_INT,    0}, \
  {"V" ,                    CONFIG_HLP_VCD,         PARAMFLAG_BOOL, iptr:&vcdflag,                         defintval:0,           TYPE_INT,    0}, \
  {"numerology" ,           CONFIG_HLP_NUMEROLOGY,  0,              iptr:&numerology,                      defintval:0,           TYPE_INT,    0}, \
  {"emulate-rf" ,           CONFIG_HLP_EMULATE_RF,  PARAMFLAG_BOOL, iptr:&emulate_rf,                      defintval:0,           TYPE_INT,    0}, \
  {"parallel-config",       CONFIG_HLP_PARALLEL_CMD,0,              strptr:(char **)&parallel_config,      defstrval:NULL,        TYPE_STRING, 0}, \
  {"worker-config",         CONFIG_HLP_WORKER_CMD,  0,              strptr:(char **)&worker_config,        defstrval:NULL,        TYPE_STRING, 0}, \
  {"s" ,                    CONFIG_HLP_SNR,         0,              dblptr:&snr_dB,                        defdblval:25,          TYPE_DOUBLE, 0}, \
  {"nbiot-disable",         CONFIG_HLP_DISABLNBIOT, PARAMFLAG_BOOL, iptr:&nonbiotflag,                     defintval:0,           TYPE_INT,    0}, \
  {"ue-timing-correction-disable", CONFIG_HLP_DISABLETIMECORR, PARAMFLAG_BOOL, iptr:&UE_no_timing_correction, defintval:0,        TYPE_INT,    0}, \
  {"rrc_config_path",       CONFIG_HLP_RRC_CFG_PATH,0,              strptr:(char **)&rrc_config_path,      defstrval:"./",        TYPE_STRING, 0} \
}


extern int T_port;
extern int T_nowait;
extern int T_dont_fork;


// In nr-ue.c
extern int setup_nr_ue_buffers(PHY_VARS_NR_UE **phy_vars_ue, openair0_config_t *openair0_cfg);
extern void fill_ue_band_info(void);
extern void init_NR_UE(int, char*);
extern void init_NR_UE_threads(int);
extern void reset_opp_meas(void);
extern void print_opp_meas(void);
void *UE_thread(void *arg);
void init_nr_ue_vars(PHY_VARS_NR_UE *ue, NR_DL_FRAME_PARMS *frame_parms, uint8_t UE_id, uint8_t abstraction_flag);
extern tpool_t *Tpool;
extern tpool_t *Tpool_dl;
#endif
