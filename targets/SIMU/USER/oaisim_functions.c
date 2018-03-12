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

/*! \file oaisim_functions.c
* \brief function primitives of oaisim
* \author Navid Nikaein
* \date 2013-2015
* \version 1.0
* \company Eurecom
* \email: openair_tech@eurecom.fr
* \note
* \warning
*/


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <execinfo.h>
#include <time.h>
#include <mcheck.h>
#include <sys/timerfd.h>

#include "assertions.h"
#include "oaisim_functions.h"

#include "PHY/extern.h"
#include "LAYER2/MAC/extern.h"
#ifdef OPENAIR2
#include "LAYER2/MAC/proto.h"
#endif
#include "LAYER2/PDCP_v10.1.0/pdcp.h"
#include "LAYER2/PDCP_v10.1.0/pdcp_primitives.h"
#include "RRC/LITE/extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "PHY_INTERFACE/extern.h"
//#include "ARCH/CBMIMO1/DEVICE_DRIVER/extern.h"
#include "SCHED/extern.h"
#include "SIMULATION/ETH_TRANSPORT/proto.h"
#include "UTIL/OCG/OCG_extern.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "UTIL/OTG/otg_config.h"
#include "UTIL/OTG/otg_tx.h"
#if ENABLE_RAL
#include "lteRALenb.h"
#include "lteRALue.h"
#endif

#include "cor_SF_sim.h"
#include "enb_config.h"

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
#endif

#if defined(ENABLE_USE_MME)
# include "s1ap_eNB.h"
#endif

#include "../../ARCH/COMMON/common_lib.h"
#include "../../ARCH/ETHERNET/USERSPACE/LIB/if_defs.h"

#include "ENB_APP/enb_paramdef.h"
#include "common/config/config_userapi.h"

#ifdef SMBV
extern uint8_t config_smbv;
extern char smbv_ip[16];
#endif

//constant for OAISIM soft realtime calibration
#define SF_DEVIATION_OFFSET_NS 100000 //= 0.1ms : should be as a number of UE
#define SLEEP_STEP_US       100 //  = 0.01ms could be adaptive, should be as a number of UE
#define K 2                  // averaging coefficient
#define TARGET_SF_TIME_NS 1000000       // 1ms = 1000000 ns


int           otg_times             = 0;
int           if_times              = 0;
int           for_times             = 0;

uint16_t           Nid_cell              = 0; //needed by init_lte_vars
int           nb_antennas_rx_ue        = 1; // //
uint8_t            target_dl_mcs         = 16; // max mcs used by MAC scheduler
uint8_t            rate_adaptation_flag  = 0;
uint8_t        set_snr         = 0;
uint8_t            set_sinr              = 0;
double             snr_dB=0, sinr_dB=0;
uint8_t            set_seed              = 0;
uint8_t            cooperation_flag;          // for cooperative communication
uint8_t            abstraction_flag      = 0;
uint8_t            ethernet_flag         = 0;
double        snr_step              = 1.0;
uint8_t            ue_connection_test    = 0;
double        forgetting_factor     = 0.0;
uint8_t            beta_ACK              = 0;
uint8_t            beta_RI               = 0;
uint8_t            beta_CQI              = 2;
uint8_t            target_ul_mcs         = 16;
//LTE_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs];
int           map1,map2;
double      **ShaF                  = NULL;
// pointers signal buffers (s = transmit, r,r0 = receive)
double      **s_re, **s_im, **r_re, **r_im, **r_re0, **r_im0;
node_list*     ue_node_list          = NULL;
node_list*     enb_node_list         = NULL;
int           omg_period            = 10000;
int           pdcp_period           = 0;
int           cba_backoff           = 30;
// time calibration for soft realtime mode
struct timespec time_spec;
unsigned long   time_last           = 0;
unsigned long   time_now            = 0;
int             td                  = 0;
int             td_avg              = 0;
int             sleep_time_us       = 0;

int phy_test = 0;

#ifdef OPENAIR2
// omv related info
//pid_t omv_pid;
char full_name[200];
extern int pfd[2]; // fd for omv : fixme: this could be a local var
char fdstr[10];
char frames[10];
char num_enb[10];
char num_ue[10];
//area_x, area_y and area_z for omv
char x_area[20];
char y_area[20];
char z_area[20];
char nb_antenna[20];
char frame_type[10];
char tdd_config[10];
#endif

Packet_OTG_List_t *otg_pdcp_buffer = NULL;

extern node_desc_t *enb_data[NUMBER_OF_RU_MAX];
extern node_desc_t *ue_data[NUMBER_OF_UE_MAX];
extern channel_desc_t *RU2UE[NUMBER_OF_RU_MAX][NUMBER_OF_UE_MAX][MAX_NUM_CCs];
extern channel_desc_t *UE2RU[NUMBER_OF_UE_MAX][NUMBER_OF_RU_MAX][MAX_NUM_CCs];

extern mapping small_scale_names[];
#if defined(Rel10) || defined(Rel14)
extern pdcp_mbms_t pdcp_mbms_array_ue[NUMBER_OF_UE_MAX][maxServiceCount][maxSessionPerPMCH];
extern pdcp_mbms_t pdcp_mbms_array_eNB[NUMBER_OF_eNB_MAX][maxServiceCount][maxSessionPerPMCH];
#endif

extern time_stats_t dl_chan_stats;
extern time_stats_t ul_chan_stats;

extern int xforms;


extern uint32_t          downlink_frequency[MAX_NUM_CCs][4];
extern int32_t           uplink_frequency_offset[MAX_NUM_CCs][4];

int oaisim_flag=1;


void RCConfig_sim(void) {

  paramlist_def_t RUParamList = {CONFIG_STRING_RU_LIST,NULL,0};


    // Get num RU instances
    config_getlist( &RUParamList,NULL,0, NULL);
    RC.nb_RU     = RUParamList.numelt;


}

void get_simulation_options(int argc, char *argv[])
{
  int                           option;

  enum long_option_e {
    LONG_OPTION_START = 0x100, /* Start after regular single char options */

    LONG_OPTION_ENB_CONF,

    LONG_OPTION_PDNC_PERIOD,
    LONG_OPTION_OMG_PERIOD,
    LONG_OPTION_OEH_ENABLED,

    LONG_OPTION_ENB_RAL_LISTENING_PORT,
    LONG_OPTION_ENB_RAL_IP_ADDRESS,
    LONG_OPTION_ENB_RAL_LINK_ID,
    LONG_OPTION_ENB_RAL_LINK_ADDRESS,

    LONG_OPTION_ENB_MIHF_REMOTE_PORT,
    LONG_OPTION_ENB_MIHF_IP_ADDRESS,
    LONG_OPTION_ENB_MIHF_ID,

    LONG_OPTION_UE_RAL_LISTENING_PORT,
    LONG_OPTION_UE_RAL_IP_ADDRESS,
    LONG_OPTION_UE_RAL_LINK_ID,
    LONG_OPTION_UE_RAL_LINK_ADDRESS,

    LONG_OPTION_UE_MIHF_REMOTE_PORT,
    LONG_OPTION_UE_MIHF_IP_ADDRESS,
    LONG_OPTION_UE_MIHF_ID,

    LONG_OPTION_MALLOC_TRACE_ENABLED,

    LONG_OPTION_CBA_BACKOFF_TIMER,

    LONG_OPTION_PHYTEST,
    LONG_OPTION_XFORMS,

#if T_TRACER
    LONG_OPTION_T_PORT,
    LONG_OPTION_T_NOWAIT,
    LONG_OPTION_T_DONT_FORK,
#endif
  };

  static struct option long_options[] = {
    {"enb-conf",               required_argument, 0, LONG_OPTION_ENB_CONF},

    {"pdcp-period",            required_argument, 0, LONG_OPTION_PDNC_PERIOD},
    {"omg-period",             required_argument, 0, LONG_OPTION_OMG_PERIOD},
    {"oeh-enabled",            no_argument, 0, LONG_OPTION_OEH_ENABLED},

    {"enb-ral-listening-port", required_argument, 0, LONG_OPTION_ENB_RAL_LISTENING_PORT},
    {"enb-ral-ip-address",     required_argument, 0, LONG_OPTION_ENB_RAL_IP_ADDRESS},
    {"enb-ral-link-id",        required_argument, 0, LONG_OPTION_ENB_RAL_LINK_ID},
    {"enb-ral-link-address",   required_argument, 0, LONG_OPTION_ENB_RAL_LINK_ADDRESS},

    {"enb-mihf-remote-port",   required_argument, 0, LONG_OPTION_ENB_MIHF_REMOTE_PORT},
    {"enb-mihf-ip-address",    required_argument, 0, LONG_OPTION_ENB_MIHF_IP_ADDRESS},
    {"enb-mihf-id",            required_argument, 0, LONG_OPTION_ENB_MIHF_ID},

    {"ue-ral-listening-port",  required_argument, 0, LONG_OPTION_UE_RAL_LISTENING_PORT},
    {"ue-ral-ip-address",      required_argument, 0, LONG_OPTION_UE_RAL_IP_ADDRESS},
    {"ue-ral-link-id",         required_argument, 0, LONG_OPTION_UE_RAL_LINK_ID},
    {"ue-ral-link-address",    required_argument, 0, LONG_OPTION_UE_RAL_LINK_ADDRESS},

    {"ue-mihf-remote-port",    required_argument, 0, LONG_OPTION_UE_MIHF_REMOTE_PORT},
    {"ue-mihf-ip-address",     required_argument, 0, LONG_OPTION_UE_MIHF_IP_ADDRESS},
    {"ue-mihf-id",             required_argument, 0, LONG_OPTION_UE_MIHF_ID},

    {"malloc-trace-enabled",   no_argument,       0, LONG_OPTION_MALLOC_TRACE_ENABLED},

    {"cba-backoff",            required_argument, 0, LONG_OPTION_CBA_BACKOFF_TIMER},

    {"phy-test", no_argument, NULL, LONG_OPTION_PHYTEST},
    {"xforms",                 no_argument,       0, LONG_OPTION_XFORMS},

#if T_TRACER
    {"T_port",                 required_argument, 0, LONG_OPTION_T_PORT},
    {"T_nowait",               no_argument,       0, LONG_OPTION_T_NOWAIT},
    {"T_dont_fork",            no_argument,       0, LONG_OPTION_T_DONT_FORK},
#endif

    {NULL, 0, NULL, 0}
  };

  while ((option = getopt_long (argc, argv, "aA:b:B:c:C:D:d:eE:f:FGg:hHi:IJ:j:k:K:l:L:m:M:n:N:oO:p:P:qQ:rR:s:S:t:T:u:U:vV:w:W:x:X:y:Y:z:Z:", long_options, NULL)) != -1) {
    switch (option) {
    case LONG_OPTION_PHYTEST:
      phy_test = 1;
      break;

    case LONG_OPTION_ENB_CONF:
      if (optarg) {
        free(RC.config_file_name); // prevent memory leak if option is used multiple times
        RC.config_file_name = strdup(optarg);
        printf("eNB configuration file is %s\n", RC.config_file_name);
      }

      break;

    case LONG_OPTION_PDNC_PERIOD:
      if (optarg) {
        pdcp_period = atoi(optarg);
        printf("PDCP period is %d\n", pdcp_period);
      }

      break;

    case LONG_OPTION_OMG_PERIOD:
      if (optarg) {
        omg_period = atoi(optarg);
        printf("OMG period is %d\n", omg_period);
      }

      break;

    case LONG_OPTION_OEH_ENABLED:
      oai_emulation.info.oeh_enabled = 1;
      break;

    case LONG_OPTION_MALLOC_TRACE_ENABLED:
      mtrace();
      break;

    case LONG_OPTION_CBA_BACKOFF_TIMER:
      oai_emulation.info.cba_backoff=atoi(optarg);
      cba_backoff=atoi(optarg);
      printf("setting CBA backoff to %d\n", cba_backoff);
      break;

#if ENABLE_RAL

    case LONG_OPTION_ENB_RAL_LISTENING_PORT:
      if (optarg) {
        g_conf_enb_ral_listening_port = strdup(optarg);
        printf("eNB RAL listening port is %s\n", g_conf_enb_ral_listening_port);
      }

      break;

    case LONG_OPTION_ENB_RAL_IP_ADDRESS:
      if (optarg) {
        g_conf_enb_ral_ip_address = strdup(optarg);
        printf("eNB RAL IP address is %s\n", g_conf_enb_ral_ip_address);
      }

      break;

    case LONG_OPTION_ENB_RAL_LINK_ADDRESS:
      if (optarg) {
        g_conf_enb_ral_link_address = strdup(optarg);
        printf("eNB RAL link address is %s\n", g_conf_enb_ral_link_address);
      }

      break;

    case LONG_OPTION_ENB_RAL_LINK_ID:
      if (optarg) {
        g_conf_enb_ral_link_id = strdup(optarg);
        printf("eNB RAL link id is %s\n", g_conf_enb_ral_link_id);
      }

      break;

    case LONG_OPTION_ENB_MIHF_REMOTE_PORT:
      if (optarg) {
        g_conf_enb_mihf_remote_port = strdup(optarg);
        printf("eNB MIH-F remote port is %s\n", g_conf_enb_mihf_remote_port);
      }

      break;

    case LONG_OPTION_ENB_MIHF_IP_ADDRESS:
      if (optarg) {
        g_conf_enb_mihf_ip_address = strdup(optarg);
        printf("eNB MIH-F IP address is %s\n", g_conf_enb_mihf_ip_address);
      }

      break;

    case LONG_OPTION_ENB_MIHF_ID:
      if (optarg) {
        g_conf_enb_mihf_id = strdup(optarg);
        printf("eNB MIH-F id is %s\n", g_conf_enb_mihf_id);
      }

      break;

    case LONG_OPTION_UE_RAL_LISTENING_PORT:
      if (optarg) {
        g_conf_ue_ral_listening_port = strdup(optarg);
        printf("UE RAL listening port is %s\n", g_conf_ue_ral_listening_port);
      }

      break;

    case LONG_OPTION_UE_RAL_IP_ADDRESS:
      if (optarg) {
        g_conf_ue_ral_ip_address = strdup(optarg);
        printf("UE RAL IP address is %s\n", g_conf_ue_ral_ip_address);
      }

      break;

    case LONG_OPTION_UE_RAL_LINK_ID:
      if (optarg) {
        g_conf_ue_ral_link_id = strdup(optarg);
        printf("UE RAL link id is %s\n", g_conf_ue_ral_link_id);
      }

      break;

    case LONG_OPTION_UE_RAL_LINK_ADDRESS:
      if (optarg) {
        g_conf_ue_ral_link_address = strdup(optarg);
        printf("UE RAL link address is %s\n", g_conf_ue_ral_link_address);
      }

      break;

    case LONG_OPTION_UE_MIHF_REMOTE_PORT:
      if (optarg) {
        g_conf_ue_mihf_remote_port = strdup(optarg);
        printf("UE MIH-F remote port is %s\n", g_conf_ue_mihf_remote_port);
      }

      break;

    case LONG_OPTION_UE_MIHF_IP_ADDRESS:
      if (optarg) {
        g_conf_ue_mihf_ip_address = strdup(optarg);
        printf("UE MIH-F IP address is %s\n", g_conf_ue_mihf_ip_address);
      }

      break;

    case LONG_OPTION_UE_MIHF_ID:
      if (optarg) {
        g_conf_ue_mihf_id = strdup(optarg);
        printf("UE MIH-F id is %s\n", g_conf_ue_mihf_id);
      }

      break;
#endif

    case LONG_OPTION_XFORMS:
      xforms=1;
      break;

#if T_TRACER
    case LONG_OPTION_T_PORT: {
      extern int T_port;
      if (optarg == NULL) abort();  /* should not happen */
      T_port = atoi(optarg);
      break;
    }

    case LONG_OPTION_T_NOWAIT: {
      extern int T_wait;
      T_wait = 0;
      break;
    }

    case LONG_OPTION_T_DONT_FORK: {
      extern int T_dont_fork;
      T_dont_fork = 1;
      break;
    }
#endif

    case 'a':
      abstraction_flag = 1;
      printf("FATAL: -a flag not functional for the moment.\nWe are working on fixing the abstraction mode.\n");
      exit(1);
      break;

    case 'A':

      //oai_emulation.info.ocm_enabled=1;
      if (optarg == NULL)
        oai_emulation.environment_system_config.fading.small_scale.selected_option="AWGN";
      else
        oai_emulation.environment_system_config.fading.small_scale.selected_option= optarg;

      //awgn_flag = 1;
      break;

    case 'b':
      oai_emulation.info.nb_enb_local = atoi (optarg);
      break;

    case 'B':
      oai_emulation.topology_config.mobility.eNB_mobility.eNB_mobility_type.selected_option = optarg;
      //oai_emulation.info.omg_model_enb = atoi (optarg);
      break;

    case 'c':
      //strcpy(oai_emulation.info.local_server, optarg);
      strncpy(oai_emulation.info.local_server, optarg, sizeof(oai_emulation.info.local_server));
      oai_emulation.info.local_server[sizeof(oai_emulation.info.local_server) - 1] = 0; // terminate string
      oai_emulation.info.ocg_enabled=1;
      break;

    case 'C':
      oai_emulation.info.tdd_config[0] = atoi (optarg);
      AssertFatal (oai_emulation.info.tdd_config[0] <= TDD_Config__subframeAssignment_sa6, "Illegal tdd_config %d (should be 0-%d)!",
                   oai_emulation.info.tdd_config[0], TDD_Config__subframeAssignment_sa6);
      break;

    case 'D':
      oai_emulation.info.multicast_ifname = strdup(optarg);
      break;

    case 'e':
      oai_emulation.info.extended_prefix_flag[0] = 1;
      break;

    case 'E':
      set_seed = 1;
      oai_emulation.info.seed = atoi (optarg);
      break;

    case 'f':
      forgetting_factor = atof (optarg);
      break;

    case 'F':                   // set FDD
      printf("Setting Frame to FDD\n");
      oai_emulation.info.frame_type[0] = 0;
      oai_emulation.info.frame_type_name[0] = "FDD";
      break;

    case 'g':
      oai_emulation.info.multicast_group = atoi (optarg);
      break;

    case 'G' :
      oai_emulation.info.otg_bg_traffic_enabled = 1;
      break;

    case 'h':
      help ();
      exit (1);
      break;

    case 'H':
      oai_emulation.info.handover_active=1;
      printf("Activate the handover procedure at RRC\n");
      break;

    case 'i':
#ifdef PROC
      Process_Flag=1;
      node_id = wgt+atoi(optarg);
      port+=atoi(optarg);
#endif
      break;

    case 'I':
      oai_emulation.info.cli_enabled = 1;
      break;

    case 'j' :
      // number of relay nodes: currently only applicable to eMBMS
      oai_emulation.info.nb_rn_local = atoi (optarg);
      break;

    case 'J':
      ue_connection_test=1;
      oai_emulation.info.ocm_enabled=0;
      snr_step = atof(optarg);
      break;

    case 'k':
      //ricean_factor = atof (optarg);
      printf("[SIM] Option k is no longer supported on the command line. Please specify your channel model in the xml template\n");
      exit(-1);
      break;

    case 'K':
      oai_emulation.info.itti_dump_file = optarg;
      break;

    case 'l':
      oai_emulation.info.g_log_level = atoi(optarg);
      break;

    case 'L':                   // set FDD
      flag_LA = atoi(optarg);
      break;

    case 'm':
      target_dl_mcs = atoi (optarg);
      printf("Max target downlink MCS used by MAC scheduler is set to %d\n", target_dl_mcs);
      break;

    case 'M':
      abstraction_flag = 1;
      ethernet_flag = 1;
      oai_emulation.info.ethernet_id = atoi (optarg);
      oai_emulation.info.master_id = oai_emulation.info.ethernet_id;
      oai_emulation.info.ethernet_flag = 1;
      break;

    case 'n':
      oai_emulation.info.n_frames = atoi (optarg);
      oai_emulation.emulation_config.emulation_time_ms= oai_emulation.info.n_frames * 10; // 10 ms frame
      //n_frames = (n_frames >1024) ? 1024: n_frames; // adjust the n_frames if higher that 1024
      oai_emulation.info.n_frames_flag = 1;
      break;

    case 'N':
      Nid_cell = atoi (optarg);

      if (Nid_cell > 503) {
        printf("Illegal Nid_cell %d (should be 0 ... 503)\n", Nid_cell);
        exit(-1);
      }

      break;

    case 'O':
      if (optarg) {
        free(RC.config_file_name); // prevent memory leak if option is used multiple times
        RC.config_file_name = strdup(optarg);
      }

      break;

    case 'o':
      oai_emulation.info.slot_isr = 1;
      break;

    case 'p':
      oai_emulation.info.nb_master = atoi (optarg);
      break;

    case 'P':
      oai_emulation.info.opt_enabled = 1;
      opt_enabled = 1;

      if (strcmp(optarg, "wireshark") == 0) {
        opt_type = OPT_WIRESHARK;
        printf("Enabling OPT for wireshark\n");
      } else if (strcmp(optarg, "pcap") == 0) {
        opt_type = OPT_PCAP;
        printf("Enabling OPT for pcap\n");
      } else {
        printf("Unrecognized option for OPT module. -> Disabling it\n");
        printf("Possible values are either wireshark or pcap\n");
        opt_type = OPT_NONE;
        oai_emulation.info.opt_enabled = 0;
        opt_enabled = 0;
      }

      oai_emulation.info.opt_mode = opt_type;
      break;

    case 'q':
      // openair performane profiler
      oai_emulation.info.opp_enabled = 1; // this var is used for OCG
      opp_enabled = 1; // this is the global var used by oaisim
      break;

    case 'Q':
      //eMBMS_active=1;
      // 0 : not used (default), 1: eMBMS and RRC enabled, 2: eMBMS relaying and RRC enabled, 3: eMBMS enabled, RRC disabled, 4: eMBMS relaying enabled, RRC disabled
      oai_emulation.info.eMBMS_active_state = atoi (optarg);
      break;

    case 'r':
      rate_adaptation_flag = 1;
      break;

    case 'R':
      oai_emulation.info.N_RB_DL[0] = atoi (optarg);

      if ((oai_emulation.info.N_RB_DL[0] != 6) && (oai_emulation.info.N_RB_DL[0] != 15) && (oai_emulation.info.N_RB_DL[0] != 25)
          && (oai_emulation.info.N_RB_DL[0] != 50) && (oai_emulation.info.N_RB_DL[0] != 75) && (oai_emulation.info.N_RB_DL[0] != 100)) {
        printf("Illegal N_RB_DL %d (should be one of 6,15,25,50,75,100)\n", oai_emulation.info.N_RB_DL[0]);
        exit (-1);
      }

      break;

    case 's':
      snr_dB = atoi (optarg);
      //      set_snr = 1;
      oai_emulation.info.ocm_enabled=0;
      break;

    case 'S':
      sinr_dB = atoi (optarg);
      set_sinr = 1;
      oai_emulation.info.ocm_enabled=0;
      break;

    case 't':
      target_ul_mcs = atoi (optarg);
      printf("Max target uplink MCS used by MAC scheduler is set to %d\n", target_ul_mcs);
      break;

    case 'T':
      oai_emulation.info.otg_enabled = 1;
      oai_emulation.info.otg_traffic = optarg;
      break;

    case 'u':
      oai_emulation.info.nb_ue_local = atoi (optarg);
      break;

    case 'U':
      oai_emulation.topology_config.mobility.UE_mobility.UE_mobility_type.selected_option = optarg;
      break;

    case 'v':
      oai_emulation.info.omv_enabled = 1;
      break;

    case 'V':
      ouput_vcd = 1;
      oai_emulation.info.vcd_enabled = 1;
      oai_emulation.info.vcd_file = optarg;
      break;

    case 'w':
      oai_emulation.info.cba_group_active = atoi (optarg);
      break;

    case 'W':
#ifdef SMBV
      config_smbv = 1;

      if(atoi(optarg)!=0)
        strcpy(smbv_ip,optarg);

#endif
      break;

    case 'x':
      /*
      oai_emulation.info.transmission_mode[0] = atoi (optarg);

      if ((oai_emulation.info.transmission_mode[0] != 1) && (oai_emulation.info.transmission_mode[0] != 2) &&
	  (oai_emulation.info.transmission_mode[0] != 3) && (oai_emulation.info.transmission_mode[0] != 4) &&
          (oai_emulation.info.transmission_mode[0] != 5) && (oai_emulation.info.transmission_mode[0] != 6)) && (oai_emulation.info.transmission_mode[0] !=7)) {
        printf("Unsupported transmission mode %d\n",oai_emulation.info.transmission_mode[0]);
        exit(-1);
      }
      */
      printf("Option -x deprecated. Please set transmission mode in eNB config file\n");
      exit(-1);

      break;

    case 'X':
#ifdef PROC
      temp=atoi(optarg);

      if(temp==0) {
        port=CHANNEL_PORT;
        Channel_Flag=1;
        Process_Flag=0;
        wgt=0;
      } else if(temp==1) {
        port=eNB_PORT;
        wgt=0;
      } else {
        port=UE_PORT;
        wgt=MAX_eNB;
      }

#endif
      break;

    case 'y':
      nb_antennas_rx_ue=atoi(optarg);

      if (nb_antennas_rx_ue>4) {
        printf("Cannot have more than 4 antennas\n");
        exit(-1);
      }

      break;

    case 'Y':
      oai_emulation.info.g_log_verbosity_option = strdup(optarg);
      break;

    case 'z':
      cooperation_flag = atoi (optarg);
      break;

    case 'Z':
      /* Sebastien ROUX: Reserved for future use (currently used in ltenow branch) */
      break;

    default:
      help ();
      exit (-1);
      break;
    }
  }

   if ( load_configmodule(argc,argv) == NULL) {
    exit_fun("[SOFTMODEM] Error, configuration module init failed\n");
  } 
 
  if (RC.config_file_name != NULL) {
    /* Read eNB configuration file */
    RCConfig_sim();
    printf("returned with %d eNBs, %d rus\n",RC.nb_inst,RC.nb_RU);
    oai_emulation.info.nb_enb_local = RC.nb_inst;
    oai_emulation.info.nb_ru_local = RC.nb_RU;
   
    /*    
    for (int j=0; j<enb_properties->nb_ru; j++) {
      
      //	local_remote_radio = BBU_REMOTE_RADIO_HEAD;
      (eth_params+j)->local_if_name             = enb_properties->ru_config[j]->ru_if_name;
      (eth_params+j)->my_addr                   = enb_properties->ru_config[j]->local_address;
      (eth_params+j)->my_port                   = enb_properties->ru_config[j]->local_port;
      (eth_params+j)->remote_addr               = enb_properties->ru_config[j]->remote_address;
      (eth_params+j)->remote_port               = enb_properties->ru_config[j]->remote_port;
      
      if (enb_properties->ru_config[j]->raw == 1) {
	(eth_params+j)->transp_preference       = ETH_RAW_MODE;
	ru_if_in[j] = REMOTE_IF5;
	node_function[j] = NGFI_RRU_IF5;
      } else if (enb_properties->ru_config[j]->rawif4p5 == 1) {
	(eth_params+j)->transp_preference       = ETH_RAW_IF4p5_MODE;
	ru_if_in[j] = REMOTE_IF4p5;    
	node_function[j] = NGFI_RRU_IF4p5; 
      } else if (enb_properties->ru_config[j]->udpif4p5 == 1) {
	(eth_params+j)->transp_preference       = ETH_UDP_IF4p5_MODE;
	ru_if_in[j] = REMOTE_IF4p5;    
	node_function[j] = NGFI_RRU_IF4p5;         
      } else if (enb_properties->ru_config[j]->rawif5_mobipass == 1) {
	(eth_params+j)->transp_preference       = ETH_RAW_IF5_MOBIPASS;             
	ru_if_in[j] = REMOTE_IF5;      
	LOG_E(EMU,"Don't use 8-bit IF5 format with oaisim, please change in configuration file\n");
      } else {
	(eth_params+j)->transp_preference       = ETH_UDP_MODE;	 
	ru_if_in[j] = REMOTE_IF5;            
	node_function[j] = NGFI_RRU_IF5;
      }
      node_timing[j] = synch_to_ext_device;

      if (enb_properties->number > 0) {
	//Update some simulation parameters 
	oai_emulation.info.frame_type[0]           = enb_properties->properties[0]->frame_type[0];
	oai_emulation.info.tdd_config[0]           = enb_properties->properties[0]->tdd_config[0];
	oai_emulation.info.tdd_config_S[0]         = enb_properties->properties[0]->tdd_config_s[0];
	oai_emulation.info.extended_prefix_flag[0] = enb_properties->properties[0]->prefix_type[0];
	
	oai_emulation.info.node_function[0]        = enb_properties->properties[0]->cc_node_function[0];
	oai_emulation.info.node_timing[0]          = enb_properties->properties[0]->cc_node_timing[0];
	downlink_frequency[0][0]                   = enb_properties->properties[0]->downlink_frequency[0];
	uplink_frequency_offset[0][0]              = enb_properties->properties[0]->uplink_frequency_offset[0];
	oai_emulation.info.N_RB_DL[0]              = enb_properties->properties[0]->N_RB_DL[0];
	LOG_E(EMU,"Please use only RRU with oaisim, remove eNB descriptors in configuration file\n");
	exit(-1);
      }
      }*/
  }
  else {
    printf("Please provide a configuration file\n");
    exit(-1);
  }
}

void check_and_adjust_params(void)
{

  int32_t ret;
  //int i,j;

  if (oai_emulation.info.nb_ue_local  + oai_emulation.info.nb_rn_local > NUMBER_OF_UE_MAX) {
    LOG_E(EMU,"Enter fewer than %d UEs/RNs for the moment or change the NUMBER_OF_UE_MAX\n", NUMBER_OF_UE_MAX);
    exit(EXIT_FAILURE);
  }

  if (oai_emulation.info.nb_enb_local + oai_emulation.info.nb_rn_local > NUMBER_OF_eNB_MAX) {
    LOG_E(EMU,"Enter fewer than %d eNBs/RNs for the moment or change the NUMBER_OF_UE_MAX\n", NUMBER_OF_eNB_MAX);
    exit(EXIT_FAILURE);
  }

  if (oai_emulation.info.nb_rn_local > NUMBER_OF_RN_MAX) {
    LOG_E(EMU,"Enter fewer than %d RNs for the moment or change the NUMBER_OF_RN_MAX\n", NUMBER_OF_RN_MAX);
    exit(EXIT_FAILURE);
  }

  // fix ethernet and abstraction with RRC_CELLULAR Flag
#ifdef RRC_CELLULAR
  abstraction_flag = 1;
  ethernet_flag = 1;
#endif

  if (set_sinr == 0)
    sinr_dB = snr_dB - 20;

  // setup netdevice interface (netlink socket)
  LOG_I(EMU,"[INIT] Starting NAS netlink interface\n");
  ret = netlink_init();

  if (ret < 0)
    LOG_W(EMU,"[INIT] Netlink not available, careful ...\n");
  /*
  if (ethernet_flag == 1) {
    oai_emulation.info.master[oai_emulation.info.master_id].nb_ue = oai_emulation.info.nb_ue_local + oai_emulation.info.nb_rn_local;
    oai_emulation.info.master[oai_emulation.info.master_id].nb_enb = oai_emulation.info.nb_enb_local + oai_emulation.info.nb_rn_local;

    if (oai_emulation.info.nb_rn_local>0)
      LOG_N(EMU,"Ethernet emulation is not yet tested with the relay nodes\n");

    if (!oai_emulation.info.master_id)
      oai_emulation.info.is_primary_master = 1;

    j = 1;

    for (i = 0; i < oai_emulation.info.nb_master; i++) {
      if (i != oai_emulation.info.master_id)
        oai_emulation.info.master_list = oai_emulation.info.master_list + j;

      LOG_I (EMU, "Index of master id i=%d  MASTER_LIST %d\n", i, oai_emulation.info.master_list);
      j *= 2;
    }

    LOG_I (EMU, " Total number of master %d my master id %d\n", oai_emulation.info.nb_master, oai_emulation.info.master_id);
    //    init_bypass ();

    while (emu_tx_status != SYNCED_TRANSPORT) {
      LOG_I (EMU, " Waiting for EMU Transport to be synced\n");
      emu_transport_sync ();    //emulation_tx_rx();
    }
  } // ethernet flag
  */
  //
  NB_UE_INST = oai_emulation.info.nb_ue_local + oai_emulation.info.nb_ue_remote;
  NB_eNB_INST = oai_emulation.info.nb_enb_local + oai_emulation.info.nb_enb_remote;
  NB_RN_INST = oai_emulation.info.nb_rn_local + oai_emulation.info.nb_rn_remote;
  NB_RU = oai_emulation.info.nb_ru_local + oai_emulation.info.nb_ru_remote;

#if defined(PDCP_USE_NETLINK_QUEUES) && defined(OPENAIR2)
  pdcp_netlink_init();
#endif

  if (NB_RN_INST > 0 ) {
    LOG_N(EMU,"Total number of RN %d (local %d, remote %d) mobility (the same as eNB) %s  \n", NB_RN_INST,oai_emulation.info.nb_rn_local,oai_emulation.info.nb_rn_remote,
          oai_emulation.topology_config.mobility.eNB_mobility.eNB_mobility_type.selected_option);

    LOG_N(EMU,"Adjust the number of eNB inst (%d->%d) and UE inst (%d->%d)\n ",
          NB_eNB_INST, NB_eNB_INST+NB_RN_INST,
          NB_UE_INST, NB_UE_INST+NB_RN_INST);
    NB_eNB_INST+=NB_RN_INST;
    NB_UE_INST+=NB_RN_INST;
  }

  LOG_I(EMU,"Total number of UE %d (first local %d , num local %d, remote %d, relay %d) mobility %s \n",
        NB_UE_INST,oai_emulation.info.first_ue_local, oai_emulation.info.nb_ue_local,oai_emulation.info.nb_ue_remote,
        NB_RN_INST,
        oai_emulation.topology_config.mobility.UE_mobility.UE_mobility_type.selected_option);

  LOG_I(EMU,"Total number of eNB %d (local %d, remote %d, relay %d) mobility %s \n",
        NB_eNB_INST,oai_emulation.info.nb_enb_local,oai_emulation.info.nb_enb_remote,
        NB_RN_INST,
        oai_emulation.topology_config.mobility.eNB_mobility.eNB_mobility_type.selected_option);

}

#ifdef OPENAIR2
void init_omv(void)
{
  if (oai_emulation.info.omv_enabled == 1) {

    if(pipe(pfd) == -1)
      perror("pipe error \n");

    snprintf( full_name, sizeof(full_name), "%s/UTIL/OMV/OMV",getenv("OPENAIR2_DIR") );
    LOG_I(EMU,"Stating the OMV path %s pfd[0] %d pfd[1] %d \n", full_name, pfd[0],pfd[1]);

    switch(fork()) {
    case -1 :
      perror("fork failed \n");
      break;

    case 0 : // child is going to be the omv, it is the reader
      if(close(pfd[1]) == -1 ) // we close the write desc.
        perror("close on write\n" );

      sprintf(fdstr, "%d", pfd[0] );
      sprintf(num_enb, "%d", NB_eNB_INST);
      sprintf(num_ue, "%d", NB_UE_INST);
      sprintf(x_area, "%f", oai_emulation.topology_config.area.x_m );
      sprintf(y_area, "%f", oai_emulation.topology_config.area.y_m );
      sprintf(z_area, "%f", 200.0 );
      sprintf(frames, "%d", oai_emulation.info.n_frames);
      sprintf(nb_antenna, "%d", 4);
      sprintf(frame_type, "%s", (oai_emulation.info.frame_type[0] == 0) ? "FDD" : "TDD");
      sprintf(tdd_config, "%d", oai_emulation.info.tdd_config[0]);
      // execl is used to launch the visualisor
      execl(full_name,"OMV", fdstr, frames, num_enb, num_ue, x_area, y_area, z_area, nb_antenna, frame_type, tdd_config,NULL );
      perror( "error in execl the OMV" );
    }

    //parent
    if(close( pfd[0] ) == -1 ) // we close the write desc.
      perror("close on read\n" );
  }
}
#endif

void init_seed(uint8_t set_seed)
{

  if(set_seed) {

    randominit (oai_emulation.info.seed);
    set_taus_seed (oai_emulation.info.seed);

  } else {
    randominit (0);
    set_taus_seed (0);
  }
}

openair0_timestamp current_ru_rx_timestamp[NUMBER_OF_RU_MAX][MAX_NUM_CCs];
openair0_timestamp current_UE_rx_timestamp[NUMBER_OF_UE_MAX][MAX_NUM_CCs];
openair0_timestamp last_ru_rx_timestamp[NUMBER_OF_RU_MAX][MAX_NUM_CCs];
openair0_timestamp last_UE_rx_timestamp[NUMBER_OF_UE_MAX][MAX_NUM_CCs];

int ru_trx_start(openair0_device *device) {
  return(0);
}

void ru_trx_end(openair0_device *device) {
  return;
}

int ru_trx_stop(openair0_device *device) {
  return(0);
}
int UE_trx_start(openair0_device *device) {
  return(0);
}
void UE_trx_end(openair0_device *device) {
  return;
}
int UE_trx_stop(openair0_device *device) {
  return(0);
}
int ru_trx_set_freq(openair0_device *device, openair0_config_t *openair0_cfg, int dummy) {
  return(0);
}
int ru_trx_set_gains(openair0_device *device, openair0_config_t *openair0_cfg) {
  return(0);
}
int UE_trx_set_freq(openair0_device *device, openair0_config_t *openair0_cfg, int dummy) {
  return(0);
}
int UE_trx_set_gains(openair0_device *device, openair0_config_t *openair0_cfg) {
  return(0);
}

extern pthread_mutex_t subframe_mutex;
extern int subframe_ru_mask,subframe_UE_mask;


int ru_trx_read(openair0_device *device, openair0_timestamp *ptimestamp, void **buff, int nsamps, int cc) {

  int ru_id  = device->Mod_id;
  int CC_id  = device->CC_id;

  int subframe;
  int sample_count=0;

  *ptimestamp = last_ru_rx_timestamp[ru_id][CC_id];


  LOG_D(EMU,"RU_trx_read nsamps %d TS(%llu,%llu) => subframe %d\n",nsamps,
        (unsigned long long)current_ru_rx_timestamp[ru_id][CC_id],
        (unsigned long long)last_ru_rx_timestamp[ru_id][CC_id],
	(int)((*ptimestamp/RC.ru[ru_id]->frame_parms.samples_per_tti)%10));
  // if we're at a subframe boundary generate UL signals for this ru

  while (sample_count<nsamps) {
    while (current_ru_rx_timestamp[ru_id][CC_id]<
	   (nsamps+last_ru_rx_timestamp[ru_id][CC_id])) {
      LOG_D(EMU,"RU: current TS %"PRIi64", last TS %"PRIi64", sleeping\n",current_ru_rx_timestamp[ru_id][CC_id],last_ru_rx_timestamp[ru_id][CC_id]);
      usleep(500);
    }

    
    subframe = (last_ru_rx_timestamp[ru_id][CC_id]/RC.ru[ru_id]->frame_parms.samples_per_tti)%10;
    LOG_D(EMU,"RU_trx_read generating UL subframe %d (Ts %llu, current TS %llu)\n",
	  subframe,(unsigned long long)*ptimestamp,
	  (unsigned long long)current_ru_rx_timestamp[ru_id][CC_id]);
    
    do_UL_sig(UE2RU,
	      enb_data,
	      ue_data,
	      subframe,
	      0,  // abstraction_flag
	      &RC.ru[ru_id]->frame_parms,
	      0,  // frame is only used for abstraction
	      ru_id,
	      CC_id);
  
    last_ru_rx_timestamp[ru_id][CC_id] += RC.ru[ru_id]->frame_parms.samples_per_tti;
    sample_count += RC.ru[ru_id]->frame_parms.samples_per_tti;
  }
  

  return(nsamps);
}

int UE_trx_read(openair0_device *device, openair0_timestamp *ptimestamp, void **buff, int nsamps, int cc)
{
  int UE_id = device->Mod_id;
  int CC_id  = device->CC_id;

  int subframe;
  int sample_count=0;
  int read_size;
  int sptti = PHY_vars_UE_g[UE_id][CC_id]->frame_parms.samples_per_tti;

  *ptimestamp = last_UE_rx_timestamp[UE_id][CC_id];

  LOG_D(EMU,"UE %d DL simulation 0: UE_trx_read nsamps %d TS %llu (%llu, offset %d) antenna %d\n",
        UE_id,
        nsamps,
        (unsigned long long)current_UE_rx_timestamp[UE_id][CC_id],
        (unsigned long long)last_UE_rx_timestamp[UE_id][CC_id],
        (int)(last_UE_rx_timestamp[UE_id][CC_id]%sptti),
	cc);


  if (nsamps < sptti)
    read_size = nsamps;
  else
    read_size = sptti;

  while (sample_count<nsamps) {
    LOG_D(EMU,"UE %d: DL simulation 1: UE_trx_read : current TS now %"PRIi64", last TS %"PRIi64"\n",UE_id,current_UE_rx_timestamp[UE_id][CC_id],last_UE_rx_timestamp[UE_id][CC_id]);
    while (current_UE_rx_timestamp[UE_id][CC_id] < 
	   (last_UE_rx_timestamp[UE_id][CC_id]+read_size)) {
      LOG_D(EMU,"UE %d: DL simulation 2: UE_trx_read : current TS %"PRIi64", last TS %"PRIi64", sleeping\n",UE_id,current_UE_rx_timestamp[UE_id][CC_id],last_UE_rx_timestamp[UE_id][CC_id]);
      usleep(500);
    }
    LOG_D(EMU,"UE %d: DL simulation 3: UE_trx_read : current TS now %"PRIi64", last TS %"PRIi64"\n",UE_id,current_UE_rx_timestamp[UE_id][CC_id],last_UE_rx_timestamp[UE_id][CC_id]);

    // if we cross a subframe-boundary
    subframe = (last_UE_rx_timestamp[UE_id][CC_id]/sptti)%10;

    // tell top-level we are busy 
    pthread_mutex_lock(&subframe_mutex);
    subframe_UE_mask|=(1<<UE_id);
    LOG_D(EMU,"Setting UE_id %d mask to busy (%d)\n",UE_id,subframe_UE_mask);
    pthread_mutex_unlock(&subframe_mutex);
    
    

    LOG_D(PHY,"UE %d: DL simulation 4: UE_trx_read generating DL subframe %d (Ts %llu, current TS %llu,nsamps %d)\n",
	  UE_id,subframe,(unsigned long long)*ptimestamp,
	  (unsigned long long)current_UE_rx_timestamp[UE_id][CC_id],
	  nsamps);

    LOG_D(EMU,"UE %d: DL simulation 5: Doing DL simulation for %d samples starting in subframe %d at offset %d\n",
	  UE_id,nsamps,subframe,
	  (int)(last_UE_rx_timestamp[UE_id][CC_id]%sptti));

    do_DL_sig(RU2UE,
	      enb_data,
	      ue_data,
	      subframe,
	      last_UE_rx_timestamp[UE_id][CC_id]%sptti,
	      sptti,
	      0, //abstraction_flag,
	      &PHY_vars_UE_g[UE_id][CC_id]->frame_parms,
	      UE_id,
	      CC_id);
    LOG_D(EMU,"UE %d: DL simulation 6: UE_trx_read @ TS %"PRIi64" (%"PRIi64")=> frame %d, subframe %d\n",
	  UE_id, current_UE_rx_timestamp[UE_id][CC_id],
	  last_UE_rx_timestamp[UE_id][CC_id],
	  (int)((last_UE_rx_timestamp[UE_id][CC_id]/(sptti*10))&1023),
	  subframe);

    last_UE_rx_timestamp[UE_id][CC_id] += read_size;
    sample_count += read_size;
 



  }


  return(nsamps);
}

extern double ru_amp[NUMBER_OF_RU_MAX];

int ru_trx_write(openair0_device *device,openair0_timestamp timestamp, void **buff, int nsamps, int cc, int flags) {

  int ru_id = device->Mod_id;

  LTE_DL_FRAME_PARMS *frame_parms = &RC.ru[ru_id]->frame_parms;

  pthread_mutex_lock(&subframe_mutex);
  LOG_D(EMU,"[TXPATH] ru_trx_write: RU %d mask %d\n",ru_id,subframe_ru_mask);
  pthread_mutex_unlock(&subframe_mutex); 

  // compute amplitude of TX signal from first symbol in subframe
  // note: assumes that the packet is an entire subframe 

  ru_amp[ru_id] = 0;
  for (int aa=0; aa<RC.ru[ru_id]->nb_tx; aa++) {
    ru_amp[ru_id] += (double)signal_energy((int32_t*)buff[aa],frame_parms->ofdm_symbol_size)/(12*frame_parms->N_RB_DL);
  }
  ru_amp[ru_id] = sqrt(ru_amp[ru_id]);

  LOG_D(EMU,"Setting amp for RU %d to %f (%d)\n",ru_id,ru_amp[ru_id], dB_fixed((double)signal_energy((int32_t*)buff[0],frame_parms->ofdm_symbol_size)));
  // tell top-level we are done
  pthread_mutex_lock(&subframe_mutex);
  subframe_ru_mask|=(1<<ru_id);
  LOG_D(EMU,"Setting RU %d to busy\n",ru_id);
  pthread_mutex_unlock(&subframe_mutex);

  return(nsamps);
}

int UE_trx_write(openair0_device *device,openair0_timestamp timestamp, void **buff, int nsamps, int cc, int flags) {

  return(nsamps);
}

//void init_openair0(void);

//openair0_config_t openair0_cfg[MAX_CARDS];

/*
void init_openair0() {

  int card;
  int i;

  for (card=0; card<MAX_CARDS; card++) {

    openair0_cfg[card].configFilename = NULL;

    if(frame_parms[0]->N_RB_DL == 100) {
      if (frame_parms[0]->threequarter_fs) {
	openair0_cfg[card].sample_rate=23.04e6;
	openair0_cfg[card].samples_per_frame = 230400; 
	openair0_cfg[card].tx_bw = 10e6;
	openair0_cfg[card].rx_bw = 10e6;
      }
      else {
	openair0_cfg[card].sample_rate=30.72e6;
	openair0_cfg[card].samples_per_frame = 307200; 
	openair0_cfg[card].tx_bw = 10e6;
	openair0_cfg[card].rx_bw = 10e6;
      }
    } else if(frame_parms[0]->N_RB_DL == 50) {
      openair0_cfg[card].sample_rate=15.36e6;
      openair0_cfg[card].samples_per_frame = 153600;
      openair0_cfg[card].tx_bw = 5e6;
      openair0_cfg[card].rx_bw = 5e6;
    } else if (frame_parms[0]->N_RB_DL == 25) {
      openair0_cfg[card].sample_rate=7.68e6;
      openair0_cfg[card].samples_per_frame = 76800;
      openair0_cfg[card].tx_bw = 2.5e6;
      openair0_cfg[card].rx_bw = 2.5e6;
    } else if (frame_parms[0]->N_RB_DL == 6) {
      openair0_cfg[card].sample_rate=1.92e6;
      openair0_cfg[card].samples_per_frame = 19200;
      openair0_cfg[card].tx_bw = 1.5e6;
      openair0_cfg[card].rx_bw = 1.5e6;
    }

    if (frame_parms[0]->frame_type==TDD)
      openair0_cfg[card].duplex_mode = duplex_mode_TDD;
    else //FDD
      openair0_cfg[card].duplex_mode = duplex_mode_FDD;

    
    openair0_cfg[card].remote_addr    = (eth_params+card)->remote_addr;
    openair0_cfg[card].remote_port    = (eth_params+card)->remote_port;
    openair0_cfg[card].my_addr        = (eth_params+card)->my_addr;
    openair0_cfg[card].my_port        = (eth_params+card)->my_port;    
     
    
    printf("HW: Configuring card %d, nb_antennas_tx/rx %d/%d\n",card,
           RC.ru[0]->nb_tx,
           RC.ru[0]->nb_rx);
    openair0_cfg[card].Mod_id = 0;



    openair0_cfg[card].num_rb_dl=frame_parms[0]->N_RB_DL;
    openair0_cfg[card].tx_num_channels=min(2,RC.ru[0]->nb_tx);
    openair0_cfg[card].rx_num_channels=min(2,RC.ru[0]->nb_rx);

    for (i=0; i<4; i++) {

      openair0_cfg[card].rx_gain[i] = RC.ru[0]->rx_total_gain_dB;
      
      printf("Card %d, channel %d, Setting tx_gain %f, rx_gain %f, tx_freq %f, rx_freq %f\n",
             card,i, openair0_cfg[card].tx_gain[i],
             openair0_cfg[card].rx_gain[i],
             openair0_cfg[card].tx_freq[i],
             openair0_cfg[card].rx_freq[i]);
    }
  }
}
*/

void init_devices(void){


  module_id_t UE_id, ru_id;
  uint8_t CC_id;
  RU_t *ru;

  // allocate memory for RU if not already done
  if (RC.ru==NULL) RC.ru = (RU_t**)malloc(RC.nb_RU*sizeof(RU_t*));

  for (ru_id=0;ru_id<RC.nb_RU;ru_id++) {
    LOG_I(EMU,"Initiaizing rfdevice for RU %d\n",ru_id);
    if (RC.ru[ru_id]==NULL) RC.ru[ru_id] = (RU_t*)malloc(sizeof(RU_t));
    ru               = RC.ru[ru_id];
    ru->rfdevice.Mod_id             = ru_id;
    ru->rfdevice.CC_id              = 0;
    ru->rfdevice.trx_start_func     = ru_trx_start;
    ru->rfdevice.trx_read_func      = ru_trx_read;
    ru->rfdevice.trx_write_func     = ru_trx_write;
    ru->rfdevice.trx_end_func       = ru_trx_end;
    ru->rfdevice.trx_stop_func      = ru_trx_stop;
    ru->rfdevice.trx_set_freq_func  = ru_trx_set_freq;
    ru->rfdevice.trx_set_gains_func = ru_trx_set_gains;
    last_ru_rx_timestamp[ru_id][0] = 0;

  }
  if (PHY_vars_UE_g==NULL) {
    PHY_vars_UE_g = (PHY_VARS_UE ***)malloc((1+NB_UE_INST)*sizeof(PHY_VARS_UE*));
    for (UE_id=0;UE_id<NB_UE_INST;UE_id++) {
      printf("Initializing UE %d\n",UE_id);
      PHY_vars_UE_g[UE_id] = (PHY_VARS_UE **)malloc((1+MAX_NUM_CCs)*sizeof(PHY_VARS_UE*));
      for (CC_id=0;CC_id<MAX_NUM_CCs;CC_id++) {
	PHY_vars_UE_g[UE_id][CC_id] = (PHY_VARS_UE *)malloc(sizeof(PHY_VARS_UE));
	PHY_vars_UE_g[UE_id][CC_id]->rfdevice.Mod_id               = UE_id;
	PHY_vars_UE_g[UE_id][CC_id]->rfdevice.CC_id                = CC_id;
	PHY_vars_UE_g[UE_id][CC_id]->rfdevice.trx_start_func       = UE_trx_start;
	PHY_vars_UE_g[UE_id][CC_id]->rfdevice.trx_read_func        = UE_trx_read;
	PHY_vars_UE_g[UE_id][CC_id]->rfdevice.trx_write_func       = UE_trx_write;
	PHY_vars_UE_g[UE_id][CC_id]->rfdevice.trx_end_func         = UE_trx_end;
	PHY_vars_UE_g[UE_id][CC_id]->rfdevice.trx_stop_func        = UE_trx_stop;
	PHY_vars_UE_g[UE_id][CC_id]->rfdevice.trx_set_freq_func    = UE_trx_set_freq;
	PHY_vars_UE_g[UE_id][CC_id]->rfdevice.trx_set_gains_func   = UE_trx_set_gains;
	last_UE_rx_timestamp[UE_id][CC_id] = 0;



      }
    }
  }
}

void init_ocm(void)
{
  module_id_t UE_id, ru_id;
  int CC_id;

  /* Added for PHY abstraction */

  /* TODO: frame_type is unused, is it intended? */
  //char* frame_type = "unknown";
  LTE_DL_FRAME_PARMS *fp = &RC.ru[0]->frame_parms;

#if 0
  switch (fp->frame_type) {
  case FDD:
    frame_type = "FDD";
    break;

  case TDD:
    frame_type = "TDD";
    break;
  }
#endif

  if (abstraction_flag) {

    get_beta_map();
    get_MIESM_param();

    //load_pbch_desc();
  }


  for (ru_id = 0; ru_id < RC.nb_RU; ru_id++) {
    enb_data[ru_id] = (node_desc_t *)malloc(sizeof(node_desc_t));
    init_enb(enb_data[ru_id],oai_emulation.environment_system_config.antenna.eNB_antenna);
  }

  for (UE_id = 0; UE_id < NB_UE_INST; UE_id++) {
    ue_data[UE_id] = (node_desc_t *)malloc(sizeof(node_desc_t));
    init_ue(ue_data[UE_id],oai_emulation.environment_system_config.antenna.UE_antenna);
  }

  if ((oai_emulation.info.ocm_enabled == 1)&& (ethernet_flag == 0 ) &&
      (oai_emulation.environment_system_config.fading.shadowing.decorrelation_distance_m>0) &&
      (oai_emulation.environment_system_config.fading.shadowing.variance_dB>0)) {

    // init SF map here!!!
    map1 =(int)oai_emulation.topology_config.area.x_m;
    map2 =(int)oai_emulation.topology_config.area.y_m;
    ShaF = init_SF(map1,map2,oai_emulation.environment_system_config.fading.shadowing.decorrelation_distance_m,oai_emulation.environment_system_config.fading.shadowing.variance_dB);

    // size of area to generate shadow fading map
    LOG_D(EMU,"Simulation area x=%f, y=%f\n",
          oai_emulation.topology_config.area.x_m,
          oai_emulation.topology_config.area.y_m);
  }

  if (abstraction_flag == 0)
    init_channel_vars (fp, &s_re, &s_im, &r_re, &r_im, &r_re0, &r_im0);

  // initialize channel descriptors
  for (ru_id = 0; ru_id < RC.nb_RU; ru_id++) {
    for (UE_id = 0; UE_id < NB_UE_INST; UE_id++) {
      for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
        LOG_I(OCM,"Initializing channel (%s, %d) from RU %d to UE %d\n", oai_emulation.environment_system_config.fading.small_scale.selected_option,
              map_str_to_int(small_scale_names,oai_emulation.environment_system_config.fading.small_scale.selected_option), ru_id, UE_id);


        RU2UE[ru_id][UE_id][CC_id] = 
	  new_channel_desc_scm(RC.ru[ru_id]->nb_tx,
			       PHY_vars_UE_g[UE_id][CC_id]->frame_parms.nb_antennas_rx,
			       map_str_to_int(small_scale_names,oai_emulation.environment_system_config.fading.small_scale.selected_option),
			       N_RB2sampling_rate(RC.ru[ru_id]->frame_parms.N_RB_DL),
			       N_RB2channel_bandwidth(RC.ru[ru_id]->frame_parms.N_RB_DL),
			       forgetting_factor,
			       0,
			       0);
        random_channel(RU2UE[ru_id][UE_id][CC_id],abstraction_flag);
        LOG_D(OCM,"[SIM] Initializing channel (%s, %d) from UE %d to ru %d\n", oai_emulation.environment_system_config.fading.small_scale.selected_option,
              map_str_to_int(small_scale_names, oai_emulation.environment_system_config.fading.small_scale.selected_option),UE_id, ru_id);


        UE2RU[UE_id][ru_id][CC_id] = 
	  new_channel_desc_scm(PHY_vars_UE_g[UE_id][CC_id]->frame_parms.nb_antennas_tx,
			       RC.ru[ru_id]->nb_rx,
			       map_str_to_int(small_scale_names, oai_emulation.environment_system_config.fading.small_scale.selected_option),
			       N_RB2sampling_rate(RC.ru[ru_id]->frame_parms.N_RB_UL),
			       N_RB2channel_bandwidth(RC.ru[ru_id]->frame_parms.N_RB_UL),
			       forgetting_factor,
			       0,
			       0);

        random_channel(UE2RU[UE_id][ru_id][CC_id],abstraction_flag);

        // to make channel reciprocal uncomment following line instead of previous. However this only works for SISO at the moment. For MIMO the channel would need to be transposed.
        //UE2RU[UE_id][ru_id] = RU2UE[ru_id][UE_id];
      }
    }
  }
}

void init_otg_pdcp_buffer(void)
{
  module_id_t i;
  otg_pdcp_buffer = malloc((NB_UE_INST + NB_eNB_INST) * sizeof(Packet_OTG_List_t));

  for (i = 0; i < NB_UE_INST + NB_eNB_INST; i++) {
    pkt_list_init(&(otg_pdcp_buffer[i]));
    //LOG_I(EMU,"HEAD of otg_pdcp_buffer[%d] is %p\n", i, pkt_list_get_head(&(otg_pdcp_buffer[i])));
  }
}

void update_omg (frame_t frameP)
{
  module_id_t UE_id, eNB_id;
  int new_omg_model;

  if ((frameP % omg_period) == 0 ) { // call OMG every 10ms
    update_nodes(oai_emulation.info.time_s);
    display_node_list(enb_node_list);
    display_node_list(ue_node_list);

    if (oai_emulation.info.omg_model_ue >= MAX_NUM_MOB_TYPES) { // mix mobility model
      for(UE_id=oai_emulation.info.first_ue_local; UE_id<(oai_emulation.info.first_ue_local+oai_emulation.info.nb_ue_local); UE_id++) {
        new_omg_model = randomgen(STATIC,RWALK);
        LOG_D(OMG, "[UE] Node of ID %d is changing mobility generator ->%d \n", UE_id, new_omg_model);
        // reset the mobility model for a specific node
        set_new_mob_type (UE_id, UE, new_omg_model, oai_emulation.info.time_s);
      }
    }

    if (oai_emulation.info.omg_model_enb >= MAX_NUM_MOB_TYPES) {      // mix mobility model
      for (eNB_id = oai_emulation.info.first_enb_local; eNB_id < (oai_emulation.info.first_enb_local + oai_emulation.info.nb_enb_local); eNB_id++) {
        new_omg_model = randomgen (STATIC, RWALK);
        LOG_D (OMG,"[eNB] Node of ID %d is changing mobility generator ->%d \n", eNB_id, new_omg_model);
        // reset the mobility model for a specific node
        set_new_mob_type (eNB_id, eNB, new_omg_model, oai_emulation.info.time_s);
      }
    }
  }
}

void update_omg_ocm()
{

  enb_node_list=get_current_positions(oai_emulation.info.omg_model_enb, eNB, oai_emulation.info.time_s);
  ue_node_list=get_current_positions(oai_emulation.info.omg_model_ue, UE, oai_emulation.info.time_s);

}

void update_ocm()
{
  module_id_t UE_id, ru_id;
  int CC_id;



  /* check if the openair channel model is activated used for PHY abstraction : path loss*/
  if ((oai_emulation.info.ocm_enabled == 1)&& (ethernet_flag == 0 )) {
    
    for (ru_id = 0; ru_id < RC.nb_RU; ru_id++)
      enb_data[ru_id]->tx_power_dBm = RC.ru[ru_id]->frame_parms.pdsch_config_common.referenceSignalPower;
    
    for (UE_id = 0; UE_id < NB_UE_INST; UE_id++)
      ue_data[UE_id]->tx_power_dBm = PHY_vars_UE_g[UE_id][0]->tx_power_dBm[0];

    //LOG_D(OMG," extracting position of eNb...\n");
    //display_node_list(enb_node_list);
    //  display_node_list(ue_node_list);
    extract_position(enb_node_list, enb_data, RC.nb_RU);
    //extract_position_fixed_enb(enb_data, NB_eNB_INST,frame);
    //LOG_D(OMG," extracting position of UE...\n");
    //      if (oai_emulation.info.omg_model_ue == TRACE)
    extract_position(ue_node_list, ue_data, NB_UE_INST);

    /* if (frame % 50 == 0)
       LOG_N(OCM,"Path loss for TTI %d : \n", frame);
    */
    for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
      for (ru_id = 0; ru_id < RC.nb_RU; ru_id++) {
        for (UE_id = 0; UE_id < NB_UE_INST; UE_id++) {
          calc_path_loss (enb_data[ru_id], ue_data[UE_id], RU2UE[ru_id][UE_id][CC_id], oai_emulation.environment_system_config,ShaF);
          //calc_path_loss (enb_data[ru_id], ue_data[UE_id], RU2UE[ru_id][UE_id], oai_emulation.environment_system_config,0);
          UE2RU[UE_id][ru_id][CC_id]->path_loss_dB = RU2UE[ru_id][UE_id][CC_id]->path_loss_dB;
          //    if (frame % 50 == 0)
          LOG_D(OCM,"Path loss (CCid %d) between eNB %d at (%f,%f) and UE %d at (%f,%f) is %f, angle %f\n",
                CC_id,ru_id,enb_data[ru_id]->x,enb_data[ru_id]->y,UE_id,ue_data[UE_id]->x,ue_data[UE_id]->y,
                RU2UE[ru_id][UE_id][CC_id]->path_loss_dB, RU2UE[ru_id][UE_id][CC_id]->aoa);
          //double dx, dy, distance;
          //dx = enb_data[ru_id]->x - ue_data[UE_id]->x;
          //dy = enb_data[ru_id]->y - ue_data[UE_id]->y;
          //distance = sqrt(dx * dx + dy * dy);
          /*LOG_D(LOCALIZE, " OCM distance between eNB %d at (%f,%f) and UE %d at (%f,%f) is %f \n",
                  ru_id, enb_data[ru_id]->x,enb_data[ru_id]->y,
                  UE_id, ue_data[UE_id]->x,ue_data[UE_id]->y,
                  distance);*/
        }
      }
    }
  }

  else {
      for (ru_id = 0; ru_id < RC.nb_RU; ru_id++) {
        for (UE_id = 0; UE_id < NB_UE_INST; UE_id++) {
	  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {

	    AssertFatal(RU2UE[ru_id][UE_id][CC_id]!=NULL,"RU2UE[%d][%d][%d] is null\n",ru_id,UE_id,CC_id);
	    AssertFatal(UE2RU[UE_id][ru_id][CC_id]!=NULL,"UE2RU[%d][%d][%d] is null\n",UE_id,ru_id,CC_id);
	    //pathloss: -132.24 dBm/15kHz RE + target SNR - eNB TX power per RE
	    if (ru_id == (UE_id % RC.nb_RU)) {
	      RU2UE[ru_id][UE_id][CC_id]->path_loss_dB = -132.24 + snr_dB - RC.ru[ru_id]->frame_parms.pdsch_config_common.referenceSignalPower;
	      UE2RU[UE_id][ru_id][CC_id]->path_loss_dB = -132.24 + snr_dB - RC.ru[ru_id]->frame_parms.pdsch_config_common.referenceSignalPower; 
	    } else {
	      RU2UE[ru_id][UE_id][CC_id]->path_loss_dB = -132.24 + sinr_dB - RC.ru[ru_id]->frame_parms.pdsch_config_common.referenceSignalPower;
	      UE2RU[UE_id][ru_id][CC_id]->path_loss_dB = -132.24 + sinr_dB - RC.ru[ru_id]->frame_parms.pdsch_config_common.referenceSignalPower;
	    }
	    
	    LOG_D(OCM,"Path loss from eNB %d to UE %d (CCid %d)=> %f dB (eNB TX %d, SNR %f)\n",ru_id,UE_id,CC_id,
		  RU2UE[ru_id][UE_id][CC_id]->path_loss_dB,
		  RC.ru[ru_id]->frame_parms.pdsch_config_common.referenceSignalPower,snr_dB);
	    
	  }
	}
      }
  }
}

#ifdef OPENAIR2
void update_otg_eNB(module_id_t enb_module_idP, unsigned int ctime)
{

#if defined(USER_MODE) && defined(OAI_EMU)

  //int rrc_state=0;
/*
  if (oai_emulation.info.otg_enabled ==1 ) {

    int dst_id, app_id;
    Packet_otg_elt_t *otg_pkt;

    for (dst_id = 0; dst_id < NUMBER_OF_UE_MAX; dst_id++) {
      for_times += 1;

      // generate traffic if the ue is rrc reconfigured state
      //if ((rrc_state=mac_eNB_get_rrc_status(enb_module_idP, dst_id)) > 2 //RRC_CONNECTED
       {
      if (mac_eNB_get_rrc_status(enb_module_idP, oai_emulation.info.eNB_ue_module_id_to_rnti[enb_module_idP][dst_id]) > 2 ){
	if_times += 1;

        for (app_id=0; app_id<MAX_NUM_APPLICATION; app_id++) {
          otg_pkt = malloc (sizeof(Packet_otg_elt_t));

          (otg_pkt->otg_pkt).sdu_buffer = (uint8_t*) packet_gen(enb_module_idP, dst_id + NB_eNB_INST, app_id, ctime, &((otg_pkt->otg_pkt).sdu_buffer_size));

          if ((otg_pkt->otg_pkt).sdu_buffer != NULL) {
            otg_times += 1;
            (otg_pkt->otg_pkt).rb_id = DTCH-2; // app could be binded to a given DRB
            (otg_pkt->otg_pkt).module_id = enb_module_idP;
            (otg_pkt->otg_pkt).dst_id = dst_id;
            (otg_pkt->otg_pkt).is_ue = 0;
            (otg_pkt->otg_pkt).mode = PDCP_TRANSMISSION_MODE_DATA;
            //Adding the packet to the OTG-PDCP buffer
            pkt_list_add_tail_eurecom(otg_pkt, &(otg_pdcp_buffer[enb_module_idP]));
            LOG_D(EMU,"[eNB %d] ADD pkt to OTG buffer with size %d for dst %d on rb_id %d for app id %d \n",
                  (otg_pkt->otg_pkt).module_id, otg_pkt->otg_pkt.sdu_buffer_size, (otg_pkt->otg_pkt).dst_id,(otg_pkt->otg_pkt).rb_id, app_id);
          } else {
            free(otg_pkt);
            otg_pkt=NULL;
          }
        }
      }
    }

#if defined(Rel10) || defined(Rel14)
    mbms_service_id_t service_id;
    mbms_session_id_t session_id;
    rb_id_t           rb_id;

    // MBSM multicast traffic
    if (ctime >= 500 ) {// only generate when UE can receive MTCH (need to control this value)
      for (service_id = 0; service_id < 2 ; service_id++) { //maxServiceCount
        for (session_id = 0; session_id < 2; session_id++) { // maxSessionPerPMCH
          if (pdcp_mbms_array_eNB[enb_module_idP][service_id][session_id].instanciated_instance == TRUE) { // this service/session is configured

            otg_pkt = malloc (sizeof(Packet_otg_elt_t));
            // LOG_T(OTG,"multicast packet gen for (service/mch %d, session/lcid %d, rb_id %d)\n", service_id, session_id, service_id*maxSessionPerPMCH + session_id);
            rb_id = pdcp_mbms_array_eNB[enb_module_idP][service_id][session_id].rb_id;
            (otg_pkt->otg_pkt).sdu_buffer = (uint8_t*) packet_gen_multicast(enb_module_idP, session_id, ctime, &((otg_pkt->otg_pkt).sdu_buffer_size));

            if ((otg_pkt->otg_pkt).sdu_buffer != NULL) {
              (otg_pkt->otg_pkt).rb_id      = rb_id;
              (otg_pkt->otg_pkt).module_id  = enb_module_idP;
              (otg_pkt->otg_pkt).dst_id     = session_id;
              (otg_pkt->otg_pkt).is_ue      = FALSE;
              //Adding the packet to the OTG-PDCP buffer
              (otg_pkt->otg_pkt).mode       = PDCP_TRANSMISSION_MODE_TRANSPARENT;
              pkt_list_add_tail_eurecom(otg_pkt, &(otg_pdcp_buffer[enb_module_idP]));
              LOG_D(EMU, "[eNB %d] ADD packet (%p) multicast to OTG buffer for dst %d on rb_id %d\n",
                    (otg_pkt->otg_pkt).module_id, otg_pkt, (otg_pkt->otg_pkt).dst_id,(otg_pkt->otg_pkt).rb_id);
            } else {
              //LOG_I(EMU, "OTG returns null \n");
              free(otg_pkt);
              otg_pkt=NULL;
            }

*/
            // old version
            /*      // MBSM multicast traffic
            #if defined(Rel10) || defined(Rel14)
            if (frame >= 46) {// only generate when UE can receive MTCH (need to control this value)
            for (service_id = 0; service_id < 2 ; service_id++) { //maxServiceCount
            for (session_id = 0; session_id < 2; session_id++) { // maxSessionPerPMCH
            //   LOG_I(OTG,"DUY:frame %d, pdcp_mbms_array[module_id][rb_id].instanciated_instance is %d\n",frame,pdcp_mbms_array[module_id][service_id*maxSessionPerPMCH + session_id].instanciated_instance);
            if ((pdcp_mbms_array[module_idP][service_id*maxSessionPerPMCH + session_id].instanciated_instance== module_idP + 1) && (eNB_flag == 1)){ // this service/session is configured
            // LOG_T(OTG,"multicast packet gen for (service/mch %d, session/lcid %d)\n", service_id, session_id);
            // Duy add
            LOG_I(OTG, "frame %d, multicast packet gen for (service/mch %d, session/lcid %d, rb_id %d)\n",frame, service_id, session_id,service_id*maxSessionPerPMCH + session_id);
            // end Duy add
            rb_id = pdcp_mbms_array[module_id][service_id*maxSessionPerPMCH + session_id].rb_id;
            otg_pkt=(uint8_t*) packet_gen_multicast(module_idP, session_id, ctime, &pkt_size);
            if (otg_pkt != NULL) {
            LOG_D(OTG,"[eNB %d] sending a multicast packet from module %d on rab id %d (src %d, dst %d) pkt size %d\n", eNB_index, module_idP, rb_id, module_idP, session_id, pkt_size);
            pdcp_data_req(module_id, frame, eNB_flag, rb_id, RLC_MUI_UNDEFINED, RLC_SDU_CONFIRM_NO, pkt_size, otg_pkt,PDCP_TM);
            free(otg_pkt);
            }
            }
            }
            }
            } // end multicast traffic
            #endif
             */
/*

          }
        }
      }

    } // end multicast traffic
*/

#endif
  }

#else
#if 0 //  defined(EXMIMO) || defined(OAI_USRP)
  if (otg_enabled==1) {
    ctime = frame * 100;

    for (dst_id = 0; dst_id < NUMBER_OF_UE_MAX; dst_id++) {
      if (mac_get_rrc_status(eNB_index, eNB_flag, dst_id ) > 2) {
        otg_pkt = malloc (sizeof(Packet_otg_elt_t));
        (otg_pkt->otg_pkt).sdu_buffer = packet_gen(module_instP, dst_id, ctime, &pkt_size);

        if (otg_pkt != NULL) {
          rb_id = DTCH-2;
          (otg_pkt->otg_pkt).rb_id     = rb_id;
          (otg_pkt->otg_pkt).module_id = module_idP;
          (otg_pkt->otg_pkt).is_ue     = FALSE;
          (otg_pkt->otg_pkt).mode      = PDCP_TRANSMISSION_MODE_DATA;
          //Adding the packet to the OTG-PDCP buffer
          pkt_list_add_tail_eurecom(otg_pkt, &(otg_pdcp_buffer[module_idP]));
          LOG_D(EMU, "[eNB %d] ADD pkt to OTG buffer for dst %d on rb_id %d\n", (otg_pkt->otg_pkt).module_id, (otg_pkt->otg_pkt).dst_id,(otg_pkt->otg_pkt).rb_id);
        } else {
          //LOG_I(EMU, "OTG returns null \n");
          free(otg_pkt);
          otg_pkt=NULL;
        }
      }
    }
  }
#endif
}

void update_otg_UE(module_id_t ue_mod_idP, unsigned int ctime)
{
}
#endif

int init_slot_isr(void)
{
  if (oai_emulation.info.slot_isr) {
    struct itimerspec its;

    int sfd;

    sfd = timerfd_create(CLOCK_REALTIME, 0);

    if (sfd == -1) {
      LOG_E(EMU, "Failed in timerfd_create (%d:%s)\n", errno, strerror(errno));
      exit(EXIT_FAILURE);
    }

    /* Start the timer */
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 500 * 1000;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    if (timerfd_settime(sfd, TFD_TIMER_ABSTIME, &its, NULL) == -1) {
      LOG_E(EMU, "Failed in timer_settime (%d:%s)\n", errno, strerror(errno));
      exit(EXIT_FAILURE);
    }

    oai_emulation.info.slot_sfd = sfd;
  }

  return 0;
}

void wait_for_slot_isr(void)
{
  uint64_t exp;
  ssize_t res;

  if (oai_emulation.info.slot_sfd > 0) {
    res = read(oai_emulation.info.slot_sfd, &exp, sizeof(exp));

    if ((res < 0) || (res != sizeof(exp))) {
      LOG_E(EMU, "Failed in read (%d:%s)\n", errno, strerror(errno));
      exit(EXIT_FAILURE);
    }
  }
}

void exit_fun(const char* s)
{
  void *array[10];
  size_t size;

  size = backtrace(array, 10);
  backtrace_symbols_fd(array, size, 2);

  fprintf(stderr, "Error: %s. Exiting!\n",s);
  exit (-1);
}

void init_time()
{
  clock_gettime (CLOCK_REALTIME, &time_spec);
  time_now      = (unsigned long) time_spec.tv_nsec;
  td_avg        = 0;
  sleep_time_us = SLEEP_STEP_US;
  td_avg        = TARGET_SF_TIME_NS;
}

// dummy function
int oai_nfapi_rach_ind(nfapi_rach_indication_t *rach_ind) {

   return(0);

}

/*
int openair0_transport_load(openair0_device *device, openair0_config_t *openair0_cfg, eth_params_t * eth_params) {

	return(0);


}

int openair0_device_load(openair0_device *device, openair0_config_t *openair0_cfg) {

	return(0);
}
*/
