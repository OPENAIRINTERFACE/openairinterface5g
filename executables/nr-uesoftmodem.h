#ifndef NR_UESOFTMODEM_H
#define NR_UESOFTMODEM_H
#include <executables/nr-softmodem-common.h>
#include <executables/softmodem-common.h>
#include "PHY/defs_nr_UE.h"
#include "SIMULATION/ETH_TRANSPORT/proto.h"




#define  CONFIG_HLP_DLSCH_PARA             "number of threads for dlsch processing 0 for no parallelization\n"
#define  CONFIG_HLP_OFFSET_DIV             "Divisor for computing OFDM symbol offset in Rx chain (num samples in CP/<the value>). Default value is 8. To set the sample offset to 0, set this value ~ 10e6\n"
/***************************************************************************************************************************************/
/* command line options definitions, CMDLINE_XXXX_DESC macros are used to initialize paramdef_t arrays which are then used as argument
   when calling config_get or config_getlist functions                                                                                 */

#define CALIBRX_OPT       "calib-ue-rx"
#define CALIBRXMED_OPT    "calib-ue-rx-med"
#define CALIBRXBYP_OPT    "calib-ue-rx-byp"
#define DBGPRACH_OPT      "debug-ue-prach"
#define NOL2CONNECT_OPT   "no-L2-connect"
#define CALIBPRACH_OPT    "calib-prach-tx"
#define DUMPFRAME_OPT     "ue-dump-frame"

/*------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters defining UE running mode                                              */
/*   optname                     helpstr                paramflags                      XXXptr        defXXXval         type       numelt   */
/*------------------------------------------------------------------------------------------------------------------------------------------*/
#define CMDLINE_NRUEPARAMS_DESC {  \
    {"usrp-args",                CONFIG_HLP_USRP_ARGS,   0,               strptr:(char **)&usrp_args,         defstrval:"type=b200", TYPE_STRING,   0},    \
    {"single-thread-disable",    CONFIG_HLP_NOSNGLT,     PARAMFLAG_BOOL,  iptr:&single_thread_flag,           defintval:1,           TYPE_INT,    0}, \
    {"dlsch-parallel",           CONFIG_HLP_DLSCH_PARA,  0,               iptr:(int32_t *)&nrUE_params.nr_dlsch_parallel,       defintval:0,           TYPE_UINT8,  0}, \
    {"offset-divisor",           CONFIG_HLP_OFFSET_DIV,  0,               uptr:(uint32_t *)&nrUE_params.ofdm_offset_divisor,    defuintval:UINT_MAX,           TYPE_UINT32,  0}, \
    {"nr-dlsch-demod-shift",     CONFIG_HLP_DLSHIFT,     0,               iptr:(int32_t *)&nr_dlsch_demod_shift,    defintval:0,     TYPE_INT,    0}, \
    {"V" ,                       CONFIG_HLP_VCD,         PARAMFLAG_BOOL,  iptr:&vcdflag,                            defintval:0,     TYPE_INT,    0}, \
    {"rrc_config_path",          CONFIG_HLP_RRC_CFG_PATH,0,               strptr:(char **)&rrc_config_path,         defstrval:"./",  TYPE_STRING, 0} \
}


/*------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters defining UE running mode                                              */
/*   optname                     helpstr                paramflags                      XXXptr        defXXXval         type       numelt   */
/*------------------------------------------------------------------------------------------------------------------------------------------*/
#define CMDLINE_NRUE_PHYPARAMS_DESC {  \
    { CALIBRX_OPT,               CONFIG_HLP_CALUER,     0,                iptr:&rx_input_level_dBm,           defintval:0,           TYPE_INT,   0}, \
    { CALIBRXMED_OPT,            CONFIG_HLP_CALUERM,    0,                iptr:&rx_input_level_dBm,           defintval:0,           TYPE_INT,   0}, \
    { CALIBRXBYP_OPT,            CONFIG_HLP_CALUERB,    0,                iptr:&rx_input_level_dBm,           defintval:0,           TYPE_INT,   0}, \
    { DBGPRACH_OPT,              CONFIG_HLP_DBGUEPR,    PARAMFLAG_BOOL,   uptr:NULL,                          defuintval:1,          TYPE_INT,   0}, \
    { NOL2CONNECT_OPT,           CONFIG_HLP_NOL2CN,     PARAMFLAG_BOOL,   uptr:NULL,                          defuintval:1,          TYPE_INT,   0}, \
    { CALIBPRACH_OPT,            CONFIG_HLP_CALPRACH,   PARAMFLAG_BOOL,   uptr:NULL,                          defuintval:1,          TYPE_INT,   0}, \
    { DUMPFRAME_OPT,             CONFIG_HLP_DUMPFRAME,  PARAMFLAG_BOOL,   iptr:&dumpframe,                    defintval:0,           TYPE_INT,   0}, \
    {"ue-rxgain",                CONFIG_HLP_UERXG,       0,               dblptr:&(rx_gain[0][0]),            defdblval:0,           TYPE_DOUBLE,0}, \
    {"ue-rxgain-off",            CONFIG_HLP_UERXGOFF,    0,               dblptr:&rx_gain_off,                defdblval:0,           TYPE_DOUBLE,0}, \
    {"ue-txgain",                CONFIG_HLP_UETXG,       0,               dblptr:&(tx_gain[0][0]),            defdblval:0,           TYPE_DOUBLE,0}, \
    {"ue-nb-ant-rx",             CONFIG_HLP_UENANTR,     0,               u8ptr:&(fp->nb_antennas_rx),        defuintval:1,          TYPE_UINT8, 0}, \
    {"ue-nb-ant-tx",             CONFIG_HLP_UENANTT,     0,               u8ptr:&(fp->nb_antennas_tx),        defuintval:1,          TYPE_UINT8, 0}, \
    {"ue-scan-carrier",          CONFIG_HLP_UESCAN,      PARAMFLAG_BOOL,  iptr:&(UE->UE_scan_carrier),        defintval:0,           TYPE_INT,   0}, \
    {"ue-fo-compensation",       CONFIG_HLP_UEFO,        PARAMFLAG_BOOL,  iptr:&(UE->UE_fo_compensation),     defintval:0,           TYPE_INT,   0}, \
    {"ue-max-power",             NULL,                   0,               iptr:&(tx_max_power[0]),            defintval:90,          TYPE_INT,   0}, \
    {"A" ,                       CONFIG_HLP_TADV,        0,               iptr:&(UE->timing_advance),         defintval:0,           TYPE_INT,   0}, \
    {"E" ,                       CONFIG_HLP_TQFS,        PARAMFLAG_BOOL,  u8ptr:&(fp->threequarter_fs),       defintval:0,           TYPE_UINT8, 0}, \
    {"r"  ,                      CONFIG_HLP_PRB_SA,      0,               iptr:&(fp->N_RB_DL),                defintval:106,         TYPE_UINT,  0}, \
    {"s"  ,                      CONFIG_HLP_SSC,         0,               u16ptr:&(fp->ssb_start_subcarrier), defintval:516,         TYPE_UINT16,0}, \
    {"T" ,                       CONFIG_HLP_TDD,         PARAMFLAG_BOOL,  iptr:&tddflag,                      defintval:0,           TYPE_INT,   0}, \
    {"do-prb-interpolation",     CONFIG_HLP_PRBINTER,    PARAMFLAG_BOOL,  iptr:&(UE->prb_interpolation),      defintval:0,           TYPE_INT,   0}, \
    {"ue-timing-correction-disable", CONFIG_HLP_DISABLETIMECORR, PARAMFLAG_BOOL, iptr:&(UE->no_timing_correction), defintval:0,      TYPE_INT,   0}, \
}


typedef struct {
  uint64_t       optmask;   //mask to store boolean config options
  uint32_t       ofdm_offset_divisor; // Divisor for sample offset computation for each OFDM symbol
  uint8_t        nr_dlsch_parallel; // number of threads for dlsch decoding, 0 means no parallelization
  tpool_t        Tpool;             // thread pool 
} nrUE_params_t;
extern uint64_t get_nrUE_optmask(void);
extern uint64_t set_nrUE_optmask(uint64_t bitmask);
extern nrUE_params_t *get_nrUE_params(void);


// In nr-ue.c
extern int setup_nr_ue_buffers(PHY_VARS_NR_UE **phy_vars_ue, openair0_config_t *openair0_cfg);
extern void fill_ue_band_info(void);
extern void init_NR_UE(int, char*);
extern void init_NR_UE_threads(int);
extern void reset_opp_meas(void);
extern void print_opp_meas(void);
void *UE_thread(void *arg);
void init_nr_ue_vars(PHY_VARS_NR_UE *ue, uint8_t UE_id, uint8_t abstraction_flag);

#endif
