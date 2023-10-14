#ifndef NR_SOFTMODEM_H
#define NR_SOFTMODEM_H

#include <executables/nr-softmodem-common.h>

#include "PHY/defs_gNB.h"

#define DEFAULT_DLF 2680000000

/***************************************************************************************************************************************/
/* command line options definitions, CMDLINE_XXXX_DESC macros are used to initialize paramdef_t arrays which are then used as argument
   when calling config_get or config_getlist functions                                                                                 */

/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            command line parameters common to eNodeB and UE                                                                           */
/*   optname                helpstr                 paramflags        XXXptr                              defXXXval                   type         numelt               */
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
// clang-format off
#define CMDLINE_PARAMS_DESC_GNB { \
  {"single-thread-disable", CONFIG_HLP_NOSNGLT,        PARAMFLAG_BOOL,   .iptr=&single_thread_flag,           .defintval=1,                     TYPE_INT,    0},        \
  {"A" ,                    CONFIG_HLP_TADV,           0,                .uptr=&timing_advance,               .defintval=0,                     TYPE_UINT,   0},        \
  {"E" ,                    CONFIG_HLP_TQFS,           PARAMFLAG_BOOL,   .i8ptr=&threequarter_fs,             .defintval=0,                     TYPE_INT8,   0},        \
  {"m" ,                    CONFIG_HLP_DLMCS_PHYTEST,  0,                .uptr=&target_dl_mcs,                .defintval=0,                     TYPE_UINT,   0},        \
  {"l" ,                    CONFIG_HLP_DLNL_PHYTEST,   0,                .uptr=&target_dl_Nl,                 .defintval=0,                     TYPE_UINT,   0},        \
  {"L" ,                    CONFIG_HLP_ULNL_PHYTEST,   0,                .uptr=&target_ul_Nl,                 .defintval=0,                     TYPE_UINT,   0},        \
  {"t" ,                    CONFIG_HLP_ULMCS_PHYTEST,  0,                .uptr=&target_ul_mcs,                .defintval=0,                     TYPE_UINT,   0},        \
  {"M" ,                    CONFIG_HLP_DLBW_PHYTEST,   0,                .uptr=&target_dl_bw,                 .defintval=0,                     TYPE_UINT,   0},        \
  {"T" ,                    CONFIG_HLP_ULBW_PHYTEST,   0,                .uptr=&target_ul_bw,                 .defintval=0,                     TYPE_UINT,   0},        \
  {"D" ,                    CONFIG_HLP_DLBM_PHYTEST,   0,                .u64ptr=&dlsch_slot_bitmap,          .defintval=0,                     TYPE_UINT64, 0},        \
  {"U" ,                    CONFIG_HLP_ULBM_PHYTEST,   0,                .u64ptr=&ulsch_slot_bitmap,          .defintval=0,                     TYPE_UINT64, 0},        \
  {"usrp-tx-thread-config", CONFIG_HLP_USRP_THREAD,    0,                .iptr=&usrp_tx_thread,               .defstrval=0,                     TYPE_INT,    0},        \
  {"uecap_file",            CONFIG_HLP_UECAP_FILE,     0,                .strptr=&uecap_file,                 .defstrval="./uecap_ports1.xml",  TYPE_STRING, 0},        \
  {"s" ,                    CONFIG_HLP_SNR,            0,                .dblptr=&snr_dB,                     .defdblval=25,                    TYPE_DOUBLE, 0},        \
}
// clang-format on

extern uint32_t target_dl_mcs;
extern uint32_t target_dl_Nl;
extern uint32_t target_ul_Nl;
extern uint32_t target_ul_mcs;
extern uint32_t target_dl_bw;
extern uint32_t target_ul_bw;
extern uint64_t dlsch_slot_bitmap;
extern uint64_t ulsch_slot_bitmap;
extern char *uecap_file;

// In nr-gnb.c
extern void init_gNB(int single_thread_flag,int wait_for_sync);
extern void stop_gNB(int);

// In nr-ru.c
extern void init_NR_RU(configmodule_interface_t *cfg, char *);
extern void init_RU_proc(RU_t *ru);
extern void start_NR_RU(void);
extern void stop_RU(int nb_ru);
extern void kill_NR_RU_proc(int inst);
extern void set_function_spec_param(RU_t *ru);

extern void reset_opp_meas(void);
extern void print_opp_meas(void);
void init_gNB_afterRU(void);

void init_pdcp(void);

#endif
