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

/*! \file oaisim.c
 * \brief oaisim top level
 * \author Navid Nikaein 
 * \date 2013-2015
 * \version 1.0
 * \company Eurecom
 * \email: openair_tech@eurecom.fr
 * \note
 * \warning
 */

#include <string.h>
#include <math.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cblas.h>
#include <execinfo.h>

#include "event_handler.h"
#include "SIMULATION/RF/defs.h"
#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/LTE_TRANSPORT/proto.h"
#include "PHY/vars.h"

#include "SIMULATION/ETH_TRANSPORT/proto.h"

//#ifdef OPENAIR2
#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/proto.h"
#include "LAYER2/MAC/vars.h"
#include "pdcp.h"
#include "RRC/LITE/vars.h"
#include "RRC/NAS/nas_config.h"

#include "SCHED/defs.h"
#include "SCHED/vars.h"
#include "system.h"


#include "PHY/TOOLS/lte_phy_scope.h"


#ifdef SMBV
// Rohde&Schwarz SMBV100A vector signal generator
#include "PHY/TOOLS/smbv.h"
char smbv_fname[] = "smbv_config_file.smbv";
unsigned short smbv_nframes = 4; // how many frames to configure 1,..,4
unsigned short config_frames[4] = {2,9,11,13};
unsigned char smbv_frame_cnt = 0;
uint8_t config_smbv = 0;
char smbv_ip[16];
#endif

#include "flexran_agent.h"


#include "oaisim_functions.h"

#include "oaisim.h"
#include "oaisim_config.h"
#include "UTIL/OCG/OCG_extern.h"
#include "cor_SF_sim.h"
#include "UTIL/OMG/omg_constants.h"
#include "UTIL/FIFO/pad_list.h"
#include "enb_app.h"

#include "../PROC/interface.h"
#include "../PROC/channel_sim_proc.h"
#include "../PROC/Tsync.h"
#include "../PROC/Process.h"

#include "UTIL/LOG/vcd_signal_dumper.h"
#include "UTIL/OTG/otg_kpi.h"
#include "assertions.h"

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
# include "create_tasks.h"
#endif

#include "T.h"

/*
  DCI0_5MHz_TDD0_t          UL_alloc_pdu;
  DCI1A_5MHz_TDD_1_6_t      CCCH_alloc_pdu;
  DCI2_5MHz_2A_L10PRB_TDD_t DLSCH_alloc_pdu1;
  DCI2_5MHz_2A_M10PRB_TDD_t DLSCH_alloc_pdu2;
*/

#define UL_RB_ALLOC            computeRIV(lte_frame_parms->N_RB_UL,0,24)
#define CCCH_RB_ALLOC          computeRIV(lte_frame_parms->N_RB_UL,0,3)
#define RA_RB_ALLOC            computeRIV(lte_frame_parms->N_RB_UL,0,3)
#define DLSCH_RB_ALLOC         0x1fff

#define DECOR_DIST             100
#define SF_VAR                 10

//constant for OAISIM soft realtime calibration
//#define SF_DEVIATION_OFFSET_NS 100000        /*= 0.1ms : should be as a number of UE */
//#define SLEEP_STEP_US          100           /*  = 0.01ms could be adaptive, should be as a number of UE */
//#define K                      2             /* averaging coefficient */
//#define TARGET_SF_TIME_NS      1000000       /* 1ms = 1000000 ns */

uint8_t usim_test = 0;

frame_t frame = 0;
char stats_buffer[16384];
channel_desc_t *RU2UE[NUMBER_OF_RU_MAX][NUMBER_OF_UE_MAX][MAX_NUM_CCs];
channel_desc_t *UE2RU[NUMBER_OF_UE_MAX][NUMBER_OF_RU_MAX][MAX_NUM_CCs];
//Added for PHY abstraction
node_desc_t *enb_data[NUMBER_OF_RU_MAX];
node_desc_t *ue_data[NUMBER_OF_UE_MAX];

pthread_cond_t sync_cond;
pthread_mutex_t sync_mutex;
int sync_var=-1;

pthread_mutex_t subframe_mutex;
int subframe_ru_mask=0,subframe_UE_mask=0;

openair0_config_t openair0_cfg[MAX_CARDS];
uint32_t          downlink_frequency[MAX_NUM_CCs][4];
int32_t           uplink_frequency_offset[MAX_NUM_CCs][4];
openair0_rf_map rf_map[MAX_NUM_CCs];

#if defined(ENABLE_ITTI)
volatile int             start_eNB = 0;
volatile int             start_UE = 0;
#endif
volatile int                    oai_exit = 0;


//int32_t **rxdata;
//int32_t **txdata;

uint16_t sf_ahead=4;
uint8_t nfapi_mode = 0;

// Added for PHY abstraction
extern node_list* ue_node_list;
extern node_list* enb_node_list;
extern int pdcp_period, omg_period;

extern double **s_re, **s_im, **r_re, **r_im, **r_re0, **r_im0;
int map1, map2;
extern double **ShaF;
double snr_dB, sinr_dB, snr_direction; //,sinr_direction;
extern double snr_step;
extern uint8_t set_sinr;
extern uint8_t ue_connection_test;
extern uint8_t set_seed;
extern uint8_t target_dl_mcs;
extern uint8_t target_ul_mcs;
extern uint8_t abstraction_flag;
extern uint8_t ethernet_flag;
extern uint16_t Nid_cell;


double cpuf;
#include "threads_t.h"
threads_t threads= {-1,-1,-1,-1,-1,-1,-1};

//#ifdef XFORMS
int otg_enabled;
int xforms=0;
//#endif

time_stats_t oaisim_stats;
time_stats_t oaisim_stats_f;
time_stats_t dl_chan_stats;
time_stats_t ul_chan_stats;

// this should reflect the channel models in openair1/SIMULATION/TOOLS/defs.h
mapping small_scale_names[] = { 
  { "custom", custom }, { "SCM_A", SCM_A },
  { "SCM_B", SCM_B   }, { "SCM_C", SCM_C },
  { "SCM_D", SCM_D   }, { "EPA",   EPA   },
  { "EVA",   EVA     }, { "ETU",   ETU   },
  { "MBSFN", MBSFN },   { "Rayleigh8", Rayleigh8 },
  { "Rayleigh1", Rayleigh1 }, { "Rayleigh1_800", Rayleigh1_800 },
  { "Rayleigh1_corr", Rayleigh1_corr }, { "Rayleigh1_anticorr", Rayleigh1_anticorr },
  { "Rice8", Rice8 }, { "Rice1", Rice1 }, { "Rice1_corr", Rice1_corr },
  { "Rice1_anticorr", Rice1_anticorr }, { "AWGN", AWGN }, { NULL,-1 }
};
#if !defined(ENABLE_ITTI)
static void *
sigh (void *arg);
#endif
void
oai_shutdown (void);

void reset_opp_meas_oaisim (void);

void wait_eNBs(void)
{
  return;
}

void
help (void)
{
  printf ("Usage: oaisim -h -a -F -C tdd_config -K [log_file] -V [vcd_file] -R N_RB_DL -e -x transmission_mode -m target_dl_mcs -r(ate_adaptation) -n n_frames -s snr_dB -k ricean_factor -t max_delay -f forgetting factor -A channel_model -z cooperation_flag -u nb_local_ue -U UE mobility -b nb_local_enb -B eNB_mobility -M ethernet_flag -p nb_master -g multicast_group -l log_level -c ocg_enable -T traffic model -D multicast network device\n");

  printf ("-h provides this help message!\n");
  printf ("-a Activates PHY abstraction mode\n");
  printf ("-A set the multipath channel simulation,  options are: SCM_A, SCM_B, SCM_C, SCM_D, EPA, EVA, ETU, Rayleigh8, Rayleigh1, Rayleigh1_corr,Rayleigh1_anticorr, Rice8,, Rice1, AWGN \n");
  printf ("-b Set the number of local eNB\n");
  printf ("-B Set the mobility model for eNB, options are: STATIC, RWP, RWALK, \n");
  printf ("-c [1,2,3,4] Activate the config generator (OCG) to process the scenario descriptor, or give the scenario manually: -c template_1.xml \n");
  printf ("-C [0-6] Sets TDD configuration\n");
  printf ("-e Activates extended prefix mode\n");
  printf ("-E Random number generator seed\n");
  printf ("-f Set the forgetting factor for time-variation\n");
  printf ("-F Activates FDD transmission (TDD is default)\n");
  printf ("-g Set multicast group ID (0,1,2,3) - valid if M is set\n");
  printf ("-G Enable background traffic \n");
  printf ("-H Enable handover operation (default disabled) \n");
  printf ("-I Enable CLI interface (to connect use telnet localhost 1352)\n");
  printf ("-k Set the Ricean factor (linear)\n");
  printf ("-K [log_file] Enable ITTI logging into log_file\n");
  printf ("-l Set the global log level (8:trace, 7:debug, 6:info, 4:warn, 3:error) \n");
  printf ("-L [0-1] 0 to disable new link adaptation, 1 to enable new link adapatation\n");
  printf ("-m Gives a fixed DL mcs for eNB scheduler\n");
  printf ("-M Set the machine ID for Ethernet-based emulation\n");
  printf ("-n Set the number of frames for the simulation. 0 for no limit\n");
  printf ("-O [enb_conf_file] eNB configuration file name\n");
  printf ("-p Set the total number of machine in emulation - valid if M is set\n");
  printf ("-P [trace type] Enable protocol analyzer. Possible values for OPT:\n");
  printf ("    - wireshark: Enable tracing of layers above PHY using an UDP socket\n");
  printf ("    - pcap:      Enable tracing of layers above PHY to a pcap file\n");
  printf ("    - tshark:    Not implemented yet\n");
  printf ("-q Enable Openair performance profiler \n");
  printf ("-Q Activate and set the MBMS service: 0 : not used (default eMBMS disabled), 1: eMBMS and RRC Connection enabled, 2: eMBMS relaying and RRC Connection enabled, 3: eMBMS enabled, RRC Connection disabled, 4: eMBMS relaying enabled, RRC Connection disabled\n");
  printf ("-R [6,15,25,50,75,100] Sets N_RB_DL\n");
  printf ("-r Activates rate adaptation (DL for now)\n");
  printf ("-s snr_dB set a fixed (average) SNR, this deactivates the openair channel model generator (OCM)\n");
  printf ("-S snir_dB set a fixed (average) SNIR, this deactivates the openair channel model generator (OCM)\n");
  printf ("-t Gives a fixed UL mcs for eNB scheduler\n");
  printf ("-T activate the traffic generator. Valide options are m2m,scbr,mcbr,bcbr,auto_pilot,bicycle_race,open_arena,team_fortress,m2m_traffic,auto_pilot_l,auto_pilot_m,auto_pilot_h,auto_pilot_e,virtual_game_l,virtual_game_m,virtual_game_h,virtual_game_f,alarm_humidity,alarm_smoke,alarm_temperature,openarena_dl,openarena_ul,voip_g711,voip_g729,video_vbr_10mbps,video_vbr_4mbps,video_vbr_2mbp,video_vbr_768kbps,video_vbr_384kbps,video_vbr_192kpbs,background_users\n");
  printf ("-u Set the number of local UE\n");
  printf ("-U Set the mobility model for UE, options are: STATIC, RWP, RWALK\n");
  printf ("-V [vcd_file] Enable VCD dump into vcd_file\n");
  printf ("-w number of CBA groups, if not specified or zero, CBA is inactive\n");
#ifdef SMBV
  printf ("-W IP address to connect to Rohde&Schwarz SMBV100A and configure SMBV from config file. -W0 uses default IP 192.168.12.201\n");
#else
  printf ("-W [Rohde&Schwarz SMBV100A functions disabled. Recompile with SMBV=1]\n");
#endif
  printf ("-x deprecated. Set the transmission mode in config file!\n");
  printf ("-y Set the number of receive antennas at the UE (1 or 2)\n");
  printf ("-Y Set the global log verbosity (none, low, medium, high, full) \n");
  printf ("-z Set the cooperation flag (0 for no cooperation, 1 for delay diversity and 2 for distributed alamouti\n");
  printf ("-Z Reserved\n");
  printf ("--xforms Activate the grapical scope\n");

#if T_TRACER
  printf ("--T_port [port]    use given port\n");
  printf ("--T_nowait         don't wait for tracer, start immediately\n");
  printf ("--T_dont_fork      to ease debugging with gdb\n");
#endif
}

pthread_t log_thread;

void
log_thread_init (void)
{
  //create log_list
  //log_list_init(&log_list);
#ifndef LOG_NO_THREAD

  log_shutdown = 0;

  if ((pthread_mutex_init (&log_lock, NULL) != 0)
      || (pthread_cond_init (&log_notify, NULL) != 0)) {
    return;
  }

  if (pthread_create (&log_thread, NULL, log_thread_function, (void*) NULL)
      != 0) {
    log_thread_finalize ();
    return;
  }

#endif

}

//Call it after the last LOG call
int
log_thread_finalize (void)
{
  int err = 0;

#ifndef LOG_NO_THREAD

  if (pthread_mutex_lock (&log_lock) != 0) {
    return -1;
  }

  log_shutdown = 1;

  /* Wake up LOG thread */
  if ((pthread_cond_broadcast (&log_notify) != 0)
      || (pthread_mutex_unlock (&log_lock) != 0)) {
    err = -1;
  }

  if (pthread_join (log_thread, NULL) != 0) {
    err = -1;
  }

  if (pthread_mutex_unlock (&log_lock) != 0) {
    err = -1;
  }

  if (!err) {
    //log_list_free(&log_list);
    pthread_mutex_lock (&log_lock);
    pthread_mutex_destroy (&log_lock);
    pthread_cond_destroy (&log_notify);
  }

#endif

  return err;
}

#if defined(ENABLE_ITTI)
static void set_cli_start(module_id_t module_idP, uint8_t start)
{
  if (module_idP < NB_eNB_INST) {
    oai_emulation.info.cli_start_enb[module_idP] = start;
  } else {
    oai_emulation.info.cli_start_ue[module_idP - NB_eNB_INST] = start;
  }
}
#endif

#ifdef OPENAIR2
int omv_write(int pfd, node_list* enb_node_list, node_list* ue_node_list, Data_Flow_Unit omv_data)
{
  module_id_t i;
  omv_data.end = 0;

  //omv_data.total_num_nodes = NB_UE_INST + NB_eNB_INST;
  for (i = 0; i < NB_eNB_INST; i++) {
    if (enb_node_list != NULL) {
      omv_data.geo[i].x = (enb_node_list->node->x_pos < 0.0) ? 0.0 : enb_node_list->node->x_pos;
      omv_data.geo[i].y = (enb_node_list->node->y_pos < 0.0) ? 0.0 : enb_node_list->node->y_pos;
      omv_data.geo[i].z = 1.0;
      omv_data.geo[i].mobility_type = oai_emulation.info.omg_model_enb;
      omv_data.geo[i].node_type = 0; //eNB
      enb_node_list = enb_node_list->next;
      omv_data.geo[i].Neighbors = 0;
/*
      for (j = NB_RU; j < NB_UE_INST + NB_RU; j++) {
        if (is_UE_active (i, j - NB_RU) == 1) {
          omv_data.geo[i].Neighbor[omv_data.geo[i].Neighbors] = j;
          omv_data.geo[i].Neighbors++;
          LOG_D(
		OMG,
		"[RU %d][UE %d] is_UE_active(i,j) %d geo (x%d, y%d) num neighbors %d\n", i, j-NB_RU, is_UE_active(i,j-NB_RU), omv_data.geo[i].x, omv_data.geo[i].y, omv_data.geo[i].Neighbors);
        }
      }
*/
    }
  }

  for (i = NB_RU; i < NB_UE_INST + NB_RU; i++) {
    if (ue_node_list != NULL) {
      omv_data.geo[i].x = (ue_node_list->node->x_pos < 0.0) ? 0.0 : ue_node_list->node->x_pos;
      omv_data.geo[i].y = (ue_node_list->node->y_pos < 0.0) ? 0.0 : ue_node_list->node->y_pos;
      omv_data.geo[i].z = 1.0;
      omv_data.geo[i].mobility_type = oai_emulation.info.omg_model_ue;
      omv_data.geo[i].node_type = 1; //UE
      //trial
      omv_data.geo[i].state = 1;
      omv_data.geo[i].rnti = 88;
      omv_data.geo[i].connected_eNB = 0;
      omv_data.geo[i].RSRP = 66;
      omv_data.geo[i].RSRQ = 55;
      omv_data.geo[i].Pathloss = 44;
      omv_data.geo[i].RSSI[0] = 33;
      omv_data.geo[i].RSSI[1] = 22;

      if ((sizeof(omv_data.geo[0].RSSI) / sizeof(omv_data.geo[0].RSSI[0])) > 2) {
        omv_data.geo[i].RSSI[2] = 11;
      }

      ue_node_list = ue_node_list->next;
      omv_data.geo[i].Neighbors = 0;
/*
      for (j = 0; j < NB_RU; j++) {
        if (is_UE_active (j, i - NB_RU) == 1) {
          omv_data.geo[i].Neighbor[omv_data.geo[i].Neighbors] = j;
          omv_data.geo[i].Neighbors++;
          LOG_D(
		OMG,
		"[UE %d][RU %d] is_UE_active  %d geo (x%d, y%d) num neighbors %d\n", i-NB_RU, j, is_UE_active(j,i-NB_RU), omv_data.geo[i].x, omv_data.geo[i].y, omv_data.geo[i].Neighbors);
        }
      }
*/
    }
  }

  LOG_E(OMG, "pfd %d \n", pfd);

  if (write (pfd, &omv_data, sizeof(struct Data_Flow_Unit)) == -1)
    perror ("write omv failed");

  return 1;
}

void omv_end(int pfd, Data_Flow_Unit omv_data)
{
  omv_data.end = 1;

  if (write (pfd, &omv_data, sizeof(struct Data_Flow_Unit)) == -1)
    perror ("write omv failed");
}
#endif

#ifdef OPENAIR2
int pfd[2]; // fd for omv : fixme: this could be a local var
#endif

#ifdef OPENAIR2
static Data_Flow_Unit omv_data;
#endif //ALU
static module_id_t UE_inst = 0;
static module_id_t eNB_inst = 0;
static module_id_t ru_id;

Packet_OTG_List_t *otg_pdcp_buffer;

typedef enum l2l1_task_state_e {
  L2L1_WAITTING, L2L1_RUNNING, L2L1_TERMINATED,
} l2l1_task_state_t;

l2l1_task_state_t l2l1_state = L2L1_WAITTING;

extern openair0_timestamp current_ru_rx_timestamp[NUMBER_OF_RU_MAX][MAX_NUM_CCs];
extern openair0_timestamp current_UE_rx_timestamp[NUMBER_OF_UE_MAX][MAX_NUM_CCs];
extern openair0_timestamp last_eNB_rx_timestamp[NUMBER_OF_eNB_MAX][MAX_NUM_CCs];
extern openair0_timestamp last_UE_rx_timestamp[NUMBER_OF_UE_MAX][MAX_NUM_CCs];

/*------------------------------------------------------------------------------*/
void *
l2l1_task (void *args_p)
{

  int CC_id;

  // Framing variables
  int32_t sf;

  //char fname[64], vname[64];

  //#ifdef XFORMS
  // current status is that every UE has a DL scope for a SINGLE eNB (eNB_id=0)
  // at eNB 0, an UL scope for every UE
  FD_lte_phy_scope_ue *form_ue[MAX_NUM_CCs][NUMBER_OF_UE_MAX];
  FD_lte_phy_scope_enb *form_enb[NUMBER_OF_UE_MAX];
  char title[255];
  char xname[32] = "oaisim";
  int xargc = 1;
  char *xargv[1];
  //#endif

#undef PRINT_STATS /* this undef is to avoid gcc warnings */
#define PRINT_STATS
#ifdef PRINT_STATS
  //int len;
  FILE *UE_stats[NUMBER_OF_UE_MAX];
  FILE *UE_stats_th[NUMBER_OF_UE_MAX];
  FILE *eNB_stats[NUMBER_OF_eNB_MAX];
  FILE *eNB_avg_thr;
  FILE *eNB_l2_stats;
  char UE_stats_filename[255];
  char eNB_stats_filename[255];
  char UE_stats_th_filename[255];
  char eNB_stats_th_filename[255];
#endif


  if (xforms==1) {
    xargv[0] = xname;
    fl_initialize (&xargc, xargv, NULL, 0, 0);
    eNB_inst = 0;
    for (UE_inst = 0; UE_inst < NB_UE_INST; UE_inst++) {
      for (CC_id=0;CC_id<MAX_NUM_CCs;CC_id++) {
	// DL scope at UEs
	form_ue[CC_id][UE_inst] = create_lte_phy_scope_ue();
	sprintf (title, "LTE DL SCOPE eNB %d to UE %d CC_id %d", eNB_inst, UE_inst, CC_id);
	fl_show_form (form_ue[CC_id][UE_inst]->lte_phy_scope_ue, FL_PLACE_HOTSPOT, FL_FULLBORDER, title);

	if (PHY_vars_UE_g[UE_inst][CC_id]->use_ia_receiver == 1) {
	  fl_set_button(form_ue[CC_id][UE_inst]->button_0,1);
	  fl_set_object_label(form_ue[CC_id][UE_inst]->button_0, "IA Receiver ON");
	  fl_set_object_color(form_ue[CC_id][UE_inst]->button_0, FL_GREEN, FL_GREEN);
	}
	
      }
    }
  }


#ifdef PRINT_STATS

  for (UE_inst=0; UE_inst<NB_UE_INST; UE_inst++) {
    sprintf(UE_stats_filename,"UE_stats%d.txt",UE_inst);
    UE_stats[UE_inst] = fopen (UE_stats_filename, "w");
  }

  for (eNB_inst=0; eNB_inst<NB_eNB_INST; eNB_inst++) {
    sprintf(eNB_stats_filename,"eNB_stats%d.txt",eNB_inst);
    eNB_stats[eNB_inst] = fopen (eNB_stats_filename, "w");
  }

  if(abstraction_flag==0) {
    for (UE_inst=0; UE_inst<NB_UE_INST; UE_inst++) {
      /* TODO: transmission_mode is defined per CC, we set 0 for now */
      sprintf(UE_stats_th_filename,"UE_stats_th%d_tx%d.txt",UE_inst,oai_emulation.info.transmission_mode[0]);
      UE_stats_th[UE_inst] = fopen (UE_stats_th_filename, "w");
    }

    /* TODO: transmission_mode is defined per CC, we set 0 for now */
    sprintf(eNB_stats_th_filename,"eNB_stats_th_tx%d.txt",oai_emulation.info.transmission_mode[0]);
    eNB_avg_thr = fopen (eNB_stats_th_filename, "w");
  } else {
    for (UE_inst=0; UE_inst<NB_UE_INST; UE_inst++) {
      /* TODO: transmission_mode is defined per CC, we set 0 for now */
      sprintf(UE_stats_th_filename,"UE_stats_abs_th%d_tx%d.txt",UE_inst,oai_emulation.info.transmission_mode[0]);
      UE_stats_th[UE_inst] = fopen (UE_stats_th_filename, "w");
    }

    /* TODO: transmission_mode is defined per CC, we set 0 for now */
    sprintf(eNB_stats_th_filename,"eNB_stats_abs_th_tx%d.txt",oai_emulation.info.transmission_mode[0]);
    eNB_avg_thr = fopen (eNB_stats_th_filename, "w");
  }

#ifdef OPENAIR2
  eNB_l2_stats = fopen ("eNB_l2_stats.txt", "w");
  LOG_I(EMU,"eNB_l2_stats=%p\n", eNB_l2_stats);
#endif

#endif

#if defined(ENABLE_ITTI)
  MessageDef *message_p = NULL;
  const char *msg_name = NULL;
  int result;

  itti_mark_task_ready (TASK_L2L1);
  LOG_I(EMU, "TASK_L2L1 is READY\n");

  if ((oai_emulation.info.nb_enb_local > 0) && 
      (oai_emulation.info.node_function[0] < NGFI_RAU_IF4p5)) {
    /* Wait for the initialize message */
    do {
      if (message_p != NULL) {
        result = itti_free (ITTI_MSG_ORIGIN_ID(message_p), message_p);
        AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
      }

      itti_receive_msg (TASK_L2L1, &message_p);
      msg_name = ITTI_MSG_NAME (message_p);
      LOG_I(EMU, "TASK_L2L1 received %s in state L2L1_WAITTING\n", msg_name);

      switch (ITTI_MSG_ID(message_p)) {
      case INITIALIZE_MESSAGE:
        l2l1_state = L2L1_RUNNING;
        start_eNB = 1;
        break;

      case ACTIVATE_MESSAGE:
        set_cli_start(ITTI_MSG_INSTANCE (message_p), 1);
        break;

      case DEACTIVATE_MESSAGE:
        set_cli_start(ITTI_MSG_INSTANCE (message_p), 0);
        break;

      case TERMINATE_MESSAGE:
        l2l1_state = L2L1_TERMINATED;
        break;

      default:
        LOG_E(EMU, "Received unexpected message %s\n", ITTI_MSG_NAME(message_p));
        break;
      }
    } while (l2l1_state == L2L1_WAITTING);

    result = itti_free (ITTI_MSG_ORIGIN_ID(message_p), message_p);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
  }

#endif
  module_id_t UE_id;

  if (abstraction_flag == 1) {
    for (UE_id = 0; UE_id < NB_UE_INST; UE_id++)
      dl_phy_sync_success (UE_id, 0, 0,1);   //UE_id%NB_eNB_INST);
  }
  
  start_meas (&oaisim_stats);

  for (frame = 0;
       (l2l1_state != L2L1_TERMINATED) && 
	 ((oai_emulation.info.n_frames_flag == 0) ||
	  (frame < oai_emulation.info.n_frames));
       frame++) {

#if defined(ENABLE_ITTI)

    do {
      // Checks if a message has been sent to L2L1 task
      itti_poll_msg (TASK_L2L1, &message_p);

      if (message_p != NULL) {
        msg_name = ITTI_MSG_NAME (message_p);
        LOG_I(EMU, "TASK_L2L1 received %s\n", msg_name);

        switch (ITTI_MSG_ID(message_p)) {
        case ACTIVATE_MESSAGE:
          set_cli_start(ITTI_MSG_INSTANCE (message_p), 1);
          break;

        case DEACTIVATE_MESSAGE:
          set_cli_start(ITTI_MSG_INSTANCE (message_p), 0);
          break;

        case TERMINATE_MESSAGE:
          l2l1_state = L2L1_TERMINATED;
          break;

        case MESSAGE_TEST:
          break;

        default:
          LOG_E(EMU, "Received unexpected message %s\n", ITTI_MSG_NAME(message_p));
          break;
        }

        result = itti_free (ITTI_MSG_ORIGIN_ID(message_p), message_p);
        AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
      }
    } while(message_p != NULL);

#endif

    //Run the aperiodic user-defined events
    if (oai_emulation.info.oeh_enabled == 1)
      execute_events (frame);

    if (ue_connection_test == 1) {
      if ((frame % 20) == 0) {
        snr_dB += snr_direction;
        sinr_dB -= snr_direction;
      }

      if (snr_dB == -20) {
        snr_direction = snr_step;
      } else if (snr_dB == 20) {
        snr_direction = -snr_step;
      }
    }

    oai_emulation.info.frame = frame;
    //oai_emulation.info.time_ms += 1;
    oai_emulation.info.time_s += 0.01; // emu time in s, each frame lasts for 10 ms // JNote: TODO check the coherency of the time and frame (I corrected it to 10 (instead of 0.01)

    update_omg (frame); // frequency is defined in the omg_global params configurable by the user
    update_omg_ocm ();

#ifdef OPENAIR2

    // check if pipe is still open
    if ((oai_emulation.info.omv_enabled == 1)) {
      omv_write (pfd[1], enb_node_list, ue_node_list, omv_data);
    }

#endif



    for (sf = 0; sf < 10; sf++) {
      LOG_D(EMU,"************************* Subframe %d\n",sf);
      start_meas (&oaisim_stats_f);

      wait_for_slot_isr ();

#if defined(ENABLE_ITTI)
      itti_update_lte_time(frame % MAX_FRAME_NUMBER, sf<<1);
#endif

      oai_emulation.info.time_ms = frame * 10 + sf;

#ifdef PROC

    if(Channel_Flag==1)
      Channel_Func(s_re2,s_im2,r_re2,r_im2,r_re02,r_im02,r_re0_d,r_im0_d,r_re0_u,r_im0_u,RU2UE,UE2RU,enb_data,ue_data,abstraction_flag,frame_parms,sf<<1);

    if(Channel_Flag==0)
#endif
      { // SUBFRAME INNER PART
#if defined(ENABLE_ITTI)
        log_set_instance_type (LOG_INSTANCE_ENB);
#endif


	CC_id=0;
        int all_done=0;

        while (all_done==0) {

          pthread_mutex_lock(&subframe_mutex);
          int subframe_ru_mask_local = subframe_ru_mask;
          int subframe_UE_mask_local  = subframe_UE_mask;
          pthread_mutex_unlock(&subframe_mutex);
          LOG_D(EMU,"Frame %d, Subframe %d, NB_RU %d, NB_UE %d: Checking masks %x,%x\n",frame,sf,NB_RU,NB_UE_INST,subframe_ru_mask_local,subframe_UE_mask_local);
          if ((subframe_ru_mask_local == ((1<<NB_RU)-1)) &&
              (subframe_UE_mask_local == ((1<<NB_UE_INST)-1)))
             all_done=1;
          else
	    usleep(1500);
        }


        //clear subframe masks for next round
        pthread_mutex_lock(&subframe_mutex);
        subframe_ru_mask=0;
        subframe_UE_mask=0;
        pthread_mutex_unlock(&subframe_mutex);

        // increment timestamps
	/*
        for (ru_id = oai_emulation.info.first_enb_local;
             (ru_id
              < (oai_emulation.info.first_enb_local
                 + oai_emulation.info.nb_enb_local));
             ru_id++) {
	*/
	for (ru_id=0;ru_id<NB_RU;ru_id++) {
	  current_ru_rx_timestamp[ru_id][CC_id] += RC.ru[ru_id]->frame_parms.samples_per_tti;
	  LOG_D(EMU,"RU %d/%d: TS %"PRIi64"\n",ru_id,CC_id,current_ru_rx_timestamp[ru_id][CC_id]);
        }
        for (UE_inst = 0; UE_inst<NB_UE_INST;UE_inst++) {
	  current_UE_rx_timestamp[UE_inst][CC_id] += PHY_vars_UE_g[UE_inst][CC_id]->frame_parms.samples_per_tti;
	  LOG_D(EMU,"UE %d/%d: TS %"PRIi64"\n",UE_inst,CC_id,current_UE_rx_timestamp[UE_inst][CC_id]);
        }

        for (eNB_inst = oai_emulation.info.first_enb_local;
             (eNB_inst
              < (oai_emulation.info.first_enb_local
                 + oai_emulation.info.nb_enb_local));
             eNB_inst++) {
          if (oai_emulation.info.cli_start_enb[eNB_inst] != 0) {
        
	    /*
	    LOG_D(EMU,
		  "PHY procedures eNB %d for frame %d, subframe %d TDD %d/%d Nid_cell %d\n",
		  eNB_inst,
		  frame % MAX_FRAME_NUMBER,
		  sf,
		  PHY_vars_eNB_g[eNB_inst][0]->frame_parms.frame_type,
		  PHY_vars_eNB_g[eNB_inst][0]->frame_parms.tdd_config,
		  PHY_vars_eNB_g[eNB_inst][0]->frame_parms.Nid_cell);
            
	    */
#ifdef OPENAIR2
	    //Application: traffic gen
            update_otg_eNB (eNB_inst, oai_emulation.info.time_ms);

            //IP/OTG to PDCP and PDCP to IP operation
            //        pdcp_run (frame, 1, 0, eNB_inst); //PHY_vars_eNB_g[eNB_id]->Mod_id
#endif
           

#ifdef PRINT_STATS

            if((sf==9) && frame%10==0)
              if(eNB_avg_thr)
                fprintf(eNB_avg_thr,"%d %d\n",RC.eNB[eNB_inst][0]->proc.proc_rxtx[sf&1].frame_tx,
                        (RC.eNB[eNB_inst][0]->total_system_throughput)/((RC.eNB[eNB_inst][0]->proc.proc_rxtx[sf&1].frame_tx+1)*10));
	    /*
            if (eNB_stats[eNB_inst]) {
              len = dump_eNB_stats(RC.eNB[eNB_inst][0], stats_buffer, 0);
              rewind (eNB_stats[eNB_inst]);
              fwrite (stats_buffer, 1, len, eNB_stats[eNB_inst]);
              fflush(eNB_stats[eNB_inst]);
            }
	    */
#ifdef OPENAIR2
/*
            if (eNB_l2_stats) {
              len = dump_eNB_l2_stats (stats_buffer, 0);
              rewind (eNB_l2_stats);
              fwrite (stats_buffer, 1, len, eNB_l2_stats);
              fflush(eNB_l2_stats);
            }
*/

#endif
#endif
          }
        }// eNB_inst loop


#if defined(ENABLE_ITTI)
        log_set_instance_type (LOG_INSTANCE_UE);
#endif


	if ((sf == 0) && ((frame % MAX_FRAME_NUMBER) == 0) && (abstraction_flag == 0)
	    && (oai_emulation.info.n_frames == 1)) {
	  
	  write_output ("dlchan0.m",
			"dlch0",
			&(PHY_vars_UE_g[0][0]->common_vars.common_vars_rx_data_per_thread[0].dl_ch_estimates[0][0][0]),
			(6
			 * (PHY_vars_UE_g[0][0]->frame_parms.ofdm_symbol_size)),
			1, 1);
	  write_output ("dlchan1.m",
			"dlch1",
			&(PHY_vars_UE_g[0][0]->common_vars.common_vars_rx_data_per_thread[0].dl_ch_estimates[1][0][0]),
			(6
			 * (PHY_vars_UE_g[0][0]->frame_parms.ofdm_symbol_size)),
			1, 1);
	  write_output ("dlchan2.m",
			"dlch2",
			&(PHY_vars_UE_g[0][0]->common_vars.common_vars_rx_data_per_thread[0].dl_ch_estimates[2][0][0]),
			(6
			 * (PHY_vars_UE_g[0][0]->frame_parms.ofdm_symbol_size)),
			1, 1);
	  write_output ("pbch_rxF_comp0.m",
			"pbch_comp0",
			PHY_vars_UE_g[0][0]->pbch_vars[0]->rxdataF_comp[0],
			6 * 12 * 4, 1, 1);
	  write_output ("pbch_rxF_llr.m", "pbch_llr",
			PHY_vars_UE_g[0][0]->pbch_vars[0]->llr,
			(PHY_vars_UE_g[0][0]->frame_parms.Ncp == 0) ? 1920 : 1728, 1,
			4);
	}
    
	stop_meas (&oaisim_stats_f);
      } // SUBFRAME INNER PART


    }
    update_ocm ();
    /*
    if ((frame >= 10) && (frame <= 11) && (abstraction_flag == 0)
#ifdef PROC
	&&(Channel_Flag==0)
#endif
	) {
      sprintf (fname, "UEtxsig%d.m", frame % MAX_FRAME_NUMBER);
      sprintf (vname, "txs%d", frame % MAX_FRAME_NUMBER);
      write_output (fname,
		    vname,
		    PHY_vars_UE_g[0][0]->common_vars.txdata[0],
		    PHY_vars_UE_g[0][0]->frame_parms.samples_per_tti
		    * 10,
		    1, 1);
      sprintf (fname, "eNBtxsig%d.m", frame % MAX_FRAME_NUMBER);
      sprintf (vname, "txs%d", frame % MAX_FRAME_NUMBER);
      write_output (fname,
		    vname,
		    PHY_vars_eNB_g[0][0]->common_vars.txdata[0][0],
		    PHY_vars_UE_g[0][0]->frame_parms.samples_per_tti
		    * 10,
		    1, 1);
      sprintf (fname, "eNBtxsigF%d.m", frame % MAX_FRAME_NUMBER);
      sprintf (vname, "txsF%d", frame % MAX_FRAME_NUMBER);
      write_output (fname,
		    vname,
		    PHY_vars_eNB_g[0][0]->common_vars.txdataF[0][0],
		    PHY_vars_eNB_g[0][0]->frame_parms.symbols_per_tti
		    * PHY_vars_eNB_g[0][0]->frame_parms.ofdm_symbol_size,
		    1, 1);
      sprintf (fname, "UErxsig%d.m", frame % MAX_FRAME_NUMBER);
      sprintf (vname, "rxs%d", frame % MAX_FRAME_NUMBER);
      write_output (fname,
		    vname,
		    PHY_vars_UE_g[0][0]->common_vars.rxdata[0],
		    PHY_vars_UE_g[0][0]->frame_parms.samples_per_tti
		    * 10,
		    1, 1);
      sprintf (fname, "eNBrxsig%d.m", frame % MAX_FRAME_NUMBER);
      sprintf (vname, "rxs%d", frame % MAX_FRAME_NUMBER);
      write_output (fname,
		    vname,
		    PHY_vars_eNB_g[0][0]->common_vars.rxdata[0][0],
		    PHY_vars_UE_g[0][0]->frame_parms.samples_per_tti
		    * 10,
		    1, 1);
    }
    */
    
    //#ifdef XFORMS
    if (xforms==1) {
      eNB_inst = 0;
      
      for (UE_inst = 0; UE_inst < NB_UE_INST; UE_inst++) {
	for (CC_id=0;CC_id<MAX_NUM_CCs;CC_id++) {
	  phy_scope_UE(form_ue[CC_id][UE_inst],
		   PHY_vars_UE_g[UE_inst][CC_id],
		   eNB_inst,
		   UE_inst,
		   7);
	}
	if (RC.eNB && RC.eNB[eNB_inst] && RC.eNB[eNB_inst][0] )
	  phy_scope_eNB(form_enb[UE_inst],
			RC.eNB[eNB_inst][0],
			UE_inst);
	
      }
    }
    //#endif
    
#ifdef SMBV
    
    // Rohde&Schwarz SMBV100A vector signal generator
    if ((frame % MAX_FRAME_NUMBER == config_frames[0]) || (frame % MAX_FRAME_NUMBER == config_frames[1]) || (frame % MAX_FRAME_NUMBER == config_frames[2]) || (frame % MAX_FRAME_NUMBER == config_frames[3])) {
      smbv_frame_cnt++;
    }
    
#endif
    
  } // frame loop

  stop_meas (&oaisim_stats);
  oai_shutdown ();
  
#ifdef PRINT_STATS
  
  for (UE_inst=0; UE_inst<NB_UE_INST; UE_inst++) {
    if (UE_stats[UE_inst])
      fclose (UE_stats[UE_inst]);
    
    if(UE_stats_th[UE_inst])
      fclose (UE_stats_th[UE_inst]);
  }
  
  for (eNB_inst=0; eNB_inst<NB_eNB_INST; eNB_inst++) {
    if (eNB_stats[eNB_inst])
      fclose (eNB_stats[eNB_inst]);
  }
  
  if (eNB_avg_thr)
    fclose (eNB_avg_thr);
  
  if (eNB_l2_stats)
    fclose (eNB_l2_stats);
  
#endif
  
#if defined(ENABLE_ITTI)
  itti_terminate_tasks(TASK_L2L1);
#endif
  
  return NULL;
}

/*
 * The following two functions are meant to restart *the lte-softmodem* and are
 * here to make oaisim compile. A restart command from the controller will be
 * ignored in oaisim.
 */
int stop_L1L2(int enb_id)
{
  LOG_W(FLEXRAN_AGENT, "stop_L1L2() not supported in oaisim\n");
  return 0;
}

int restart_L1L2(int enb_id)
{
  LOG_W(FLEXRAN_AGENT, "restart_L1L2() not supported in oaisim\n");
  return 0;
}

#if T_TRACER
int T_wait = 1;       /* by default we wait for the tracer */
int T_port = 2021;    /* default port to listen to to wait for the tracer */
int T_dont_fork = 0;  /* default is to fork, see 'T_init' to understand */
#endif


void wait_RUs(void)
{
  int i;

  // wait for all RUs to be configured over fronthaul
  pthread_mutex_lock(&RC.ru_mutex);



  while (RC.ru_mask>0) {
    pthread_cond_wait(&RC.ru_cond,&RC.ru_mutex);
  }

  // copy frame parameters from RU to UEs
  for (i=0;i<NB_UE_INST;i++) {
    PHY_vars_UE_g[i][0]->frame_parms.N_RB_DL              = RC.ru[0]->frame_parms.N_RB_DL;
    PHY_vars_UE_g[i][0]->frame_parms.N_RB_UL              = RC.ru[0]->frame_parms.N_RB_UL;
    PHY_vars_UE_g[i][0]->frame_parms.nb_antennas_tx       = 1;
    PHY_vars_UE_g[i][0]->frame_parms.nb_antennas_rx       = 1;
    // set initially to 2, it will be revised after initial synchronization
    PHY_vars_UE_g[i][0]->frame_parms.nb_antenna_ports_eNB = 2;
    PHY_vars_UE_g[i][0]->frame_parms.tdd_config = 1;
    PHY_vars_UE_g[i][0]->frame_parms.dl_CarrierFreq       = RC.ru[0]->frame_parms.dl_CarrierFreq;
    PHY_vars_UE_g[i][0]->frame_parms.ul_CarrierFreq       = RC.ru[0]->frame_parms.ul_CarrierFreq;
    PHY_vars_UE_g[i][0]->frame_parms.eutra_band           = RC.ru[0]->frame_parms.eutra_band;
    LOG_I(PHY,"Initializing UE %d frame parameters from RU information: N_RB_DL %d, p %d, dl_Carrierfreq %u, ul_CarrierFreq %u, eutra_band %d\n",
	  i,
	  PHY_vars_UE_g[i][0]->frame_parms.N_RB_DL,
	  PHY_vars_UE_g[i][0]->frame_parms.nb_antenna_ports_eNB,
	  PHY_vars_UE_g[i][0]->frame_parms.dl_CarrierFreq,
	  PHY_vars_UE_g[i][0]->frame_parms.ul_CarrierFreq,
	  PHY_vars_UE_g[i][0]->frame_parms.eutra_band);

    current_UE_rx_timestamp[i][0] = RC.ru[0]->frame_parms.samples_per_tti + RC.ru[0]->frame_parms.ofdm_symbol_size + RC.ru[0]->frame_parms.nb_prefix_samples0;

  }
  
  


  for (ru_id=0;ru_id<RC.nb_RU;ru_id++) current_ru_rx_timestamp[ru_id][0] = RC.ru[ru_id]->frame_parms.samples_per_tti;

  printf("RUs are ready, let's go\n");
}

void init_UE(int,int,int,int);
void init_RU(const char*);

void set_UE_defaults(int nb_ue) {

  for (int UE_id = 0;UE_id<nb_ue;UE_id++) {
    for (int CC_id = 0;CC_id<MAX_NUM_CCs;CC_id++) {
      for (uint8_t i=0; i<RX_NB_TH_MAX; i++) {
	PHY_vars_UE_g[UE_id][CC_id]->pdcch_vars[i][0]->dciFormat      = 0;
	PHY_vars_UE_g[UE_id][CC_id]->pdcch_vars[i][0]->agregationLevel      = 0xFF;
      }
      PHY_vars_UE_g[UE_id][CC_id]->current_dlsch_cqi[0] = 10;
    }
  }
}


static void print_current_directory(void)
{
  char dir[8192]; /* arbitrary size (should be big enough) */
  if (getcwd(dir, 8192) == NULL)
    printf("ERROR getting working directory\n");
  else
    printf("working directory: %s\n", dir);
}

void init_devices(void);

int main (int argc, char **argv)
{

  clock_t t;

  print_current_directory();

  start_background_system();

#ifdef SMBV
  // Rohde&Schwarz SMBV100A vector signal generator
  strcpy(smbv_ip,DEFAULT_SMBV_IP);
#endif

#ifdef PROC
  int node_id;
  int port,Process_Flag=0,wgt,Channel_Flag=0,temp;
#endif

  //default parameters
  oai_emulation.info.n_frames = MAX_FRAME_NUMBER; //1024;          //10;
  oai_emulation.info.n_frames_flag = 0; //fixme
  snr_dB = 30;

  //Default values if not changed by the user in get_simulation_options();
  pdcp_period = 1;
  omg_period = 1;
  //Clean ip rule table
  for(int i =0; i<NUMBER_OF_UE_MAX; i++){
      char command_line[100];
      sprintf(command_line, "while ip rule del table %d; do true; done",i+201);
      /* we don't care about return value from system(), but let's the
       * compiler be silent, so let's do "if (XX);"
       */
      if (system(command_line)) /* nothing */;
  }
  // start thread for log gen
  log_thread_init ();

  init_oai_emulation (); // to initialize everything !!!

  // get command-line options
  get_simulation_options (argc, argv); //Command-line options

#if T_TRACER
  T_init(T_port, T_wait, T_dont_fork);
#endif

  // Initialize VCD LOG module
  VCD_SIGNAL_DUMPER_INIT (oai_emulation.info.vcd_file);

#if !defined(ENABLE_ITTI)
  pthread_t tid;
  int err;
  sigset_t sigblock;
  sigemptyset (&sigblock);
  sigaddset (&sigblock, SIGHUP);
  sigaddset (&sigblock, SIGINT);
  sigaddset (&sigblock, SIGTERM);
  sigaddset (&sigblock, SIGQUIT);
  //sigaddset(&sigblock, SIGKILL);

  if ((err = pthread_sigmask (SIG_BLOCK, &sigblock, NULL)) != 0) {
    printf ("SIG_BLOCK error\n");
    return -1;
  }

  if (pthread_create (&tid, NULL, sigh, NULL)) {
    printf ("Pthread for tracing Signals is not created!\n");
    return -1;
  } else {
    printf ("Pthread for tracing Signals is created!\n");
  }

#endif
  // configure oaisim with OCG
  oaisim_config (); // config OMG and OCG, OPT, OTG, OLG

  if (ue_connection_test == 1) {
    snr_direction = -snr_step;
    snr_dB = 20;
    sinr_dB = -20;
  }

  pthread_cond_init(&sync_cond,NULL);
  pthread_mutex_init(&sync_mutex, NULL);
  pthread_mutex_init(&subframe_mutex, NULL);

#ifdef OPENAIR2
  init_omv ();
#endif
  //Before this call, NB_UE_INST and NB_eNB_INST are not set correctly
  check_and_adjust_params ();

  set_seed = oai_emulation.emulation_config.seed.value;

  init_otg_pdcp_buffer ();

  init_seed (set_seed);


  init_RU(NULL);

  init_devices ();

  //  init_openair2 ();
  //  init_openair0();




  if (create_tasks_ue(oai_emulation.info.nb_ue_local) < 0) 
      exit(-1); // need a softer mode


  printf("Waiting for RUs to get set up\n"); 
  wait_RUs();

  init_UE(NB_UE_INST,0,0,1);

  set_UE_defaults(NB_UE_INST);


  init_ocm ();
  printf("Sending sync to all threads\n");


  pthread_mutex_lock(&sync_mutex);
  sync_var=0;
  pthread_cond_broadcast(&sync_cond);
  pthread_mutex_unlock(&sync_mutex);

#ifdef SMBV
  // Rohde&Schwarz SMBV100A vector signal generator
  smbv_init_config(smbv_fname, smbv_nframes);
  smbv_write_config_from_frame_parms(smbv_fname, &PHY_vars_eNB_g[0][0]->frame_parms);
#endif

  /* #if defined (FLEXRAN_AGENT_SB_IF)
  flexran_agent_start();
  #endif */ 

  // add events to future event list: Currently not used
  //oai_emulation.info.oeh_enabled = 1;
  if (oai_emulation.info.oeh_enabled == 1)
    schedule_events ();

  // oai performance profiler is enabled
  if (oai_emulation.info.opp_enabled == 1)
    reset_opp_meas_oaisim ();

  cpuf=get_cpu_freq_GHz();

  init_time ();

  init_slot_isr ();

  t = clock ();

  LOG_N(EMU,
        ">>>>>>>>>>>>>>>>>>>>>>>>>>> OAIEMU initialization done <<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

#ifndef PACKAGE_VERSION
#  define PACKAGE_VERSION "UNKNOWN-EXPERIMENTAL"
#endif
  LOG_I(EMU, "Version: %s\n", PACKAGE_VERSION);

#if defined(ENABLE_ITTI)

  // Handle signals until all tasks are terminated
  itti_wait_tasks_end();

#else

  if (oai_emulation.info.nb_enb_local > 0) {
    eNB_app_task (NULL); // do nothing for the moment
  }

  l2l1_task (NULL);
#endif
  t = clock () - t;
  LOG_I(EMU, "Duration of the simulation: %f seconds\n",
        ((float) t) / CLOCKS_PER_SEC);

  LOG_N(EMU,
        ">>>>>>>>>>>>>>>>>>>>>>>>>>> OAIEMU Ending <<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

  raise (SIGINT);
  //  oai_shutdown ();

  return (0);
}

void
reset_opp_meas_oaisim (void)
{
  uint8_t eNB_id = 0, UE_id = 0;

  reset_meas (&oaisim_stats);
  reset_meas (&oaisim_stats_f); // frame

  // init time stats here (including channel)
  reset_meas (&dl_chan_stats);
  reset_meas (&ul_chan_stats);

  for (UE_id = 0; UE_id < NB_UE_INST; UE_id++) {
    reset_meas (&PHY_vars_UE_g[UE_id][0]->phy_proc[0]);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->phy_proc[1]);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->phy_proc_rx[0]);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->phy_proc_rx[1]);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->phy_proc_tx);

    //    reset_meas (&PHY_vars_UE_g[UE_id][0]->ofdm_demod_stats);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->rx_dft_stats);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_channel_estimation_stats);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_freq_offset_estimation_stats);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_decoding_stats[0]);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_decoding_stats[1]);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_rate_unmatching_stats);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_turbo_decoding_stats);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_deinterleaving_stats);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_llr_stats);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_unscrambling_stats);

    reset_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_tc_init_stats);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_tc_alpha_stats);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_tc_beta_stats);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_tc_gamma_stats);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_tc_ext_stats);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_tc_intl1_stats);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_tc_intl2_stats);

    reset_meas (&PHY_vars_UE_g[UE_id][0]->tx_prach);

    reset_meas (&PHY_vars_UE_g[UE_id][0]->ofdm_mod_stats);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->ulsch_encoding_stats);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->ulsch_modulation_stats);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->ulsch_segmentation_stats);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->ulsch_rate_matching_stats);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->ulsch_turbo_encoding_stats);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->ulsch_interleaving_stats);
    reset_meas (&PHY_vars_UE_g[UE_id][0]->ulsch_multiplexing_stats);


    /*
     * L2 functions
     */

    // UE MAC
    reset_meas (&UE_mac_inst[UE_id].ue_scheduler); // total
    reset_meas (&UE_mac_inst[UE_id].tx_ulsch_sdu); // inlcude rlc_data_req + mac header gen
    reset_meas (&UE_mac_inst[UE_id].rx_dlsch_sdu); // include mac_rrc_data_ind or mac_rlc_status_ind+mac_rlc_data_ind and  mac header parser
    reset_meas (&UE_mac_inst[UE_id].ue_query_mch);
    reset_meas (&UE_mac_inst[UE_id].rx_mch_sdu); // include rld_data_ind+ parse mch header
    reset_meas (&UE_mac_inst[UE_id].rx_si); // include rlc_data_ind + mac header parser

    reset_meas (&UE_pdcp_stats[UE_id].pdcp_run);
    reset_meas (&UE_pdcp_stats[UE_id].data_req);
    reset_meas (&UE_pdcp_stats[UE_id].data_ind);
    reset_meas (&UE_pdcp_stats[UE_id].apply_security);
    reset_meas (&UE_pdcp_stats[UE_id].validate_security);
    reset_meas (&UE_pdcp_stats[UE_id].pdcp_ip);
    reset_meas (&UE_pdcp_stats[UE_id].ip_pdcp);

    
  }

  for (eNB_id = 0; eNB_id < NB_eNB_INST; eNB_id++) {

    for (UE_id = 0; UE_id < NB_UE_INST; UE_id++) {
      reset_meas (&RU2UE[eNB_id][UE_id][0]->random_channel);
      reset_meas (&RU2UE[eNB_id][UE_id][0]->interp_time);
      reset_meas (&RU2UE[eNB_id][UE_id][0]->interp_freq);
      reset_meas (&RU2UE[eNB_id][UE_id][0]->convolution);
      reset_meas (&UE2RU[UE_id][eNB_id][0]->random_channel);
      reset_meas (&UE2RU[UE_id][eNB_id][0]->interp_time);
      reset_meas (&UE2RU[UE_id][eNB_id][0]->interp_freq);
      reset_meas (&UE2RU[UE_id][eNB_id][0]->convolution);
    }

    reset_meas (&RC.eNB[eNB_id][0]->phy_proc);
    reset_meas (&RC.eNB[eNB_id][0]->phy_proc_rx);
    reset_meas (&RC.eNB[eNB_id][0]->phy_proc_tx);
    reset_meas (&RC.eNB[eNB_id][0]->rx_prach);

    reset_meas (&RC.eNB[eNB_id][0]->ofdm_mod_stats);
    reset_meas (&RC.eNB[eNB_id][0]->dlsch_encoding_stats);
    reset_meas (&RC.eNB[eNB_id][0]->dlsch_modulation_stats);
    reset_meas (&RC.eNB[eNB_id][0]->dlsch_scrambling_stats);
    reset_meas (&RC.eNB[eNB_id][0]->dlsch_rate_matching_stats);
    reset_meas (&RC.eNB[eNB_id][0]->dlsch_turbo_encoding_stats);
    reset_meas (&RC.eNB[eNB_id][0]->dlsch_interleaving_stats);

    //    reset_meas (&RC.eNB[eNB_id][0]->ofdm_demod_stats);
    //reset_meas(&RC.eNB[eNB_id]->rx_dft_stats);
    //reset_meas(&RC.eNB[eNB_id]->ulsch_channel_estimation_stats);
    //reset_meas(&RC.eNB[eNB_id]->ulsch_freq_offset_estimation_stats);
    reset_meas (&RC.eNB[eNB_id][0]->ulsch_decoding_stats);
    reset_meas (&RC.eNB[eNB_id][0]->ulsch_demodulation_stats);
    reset_meas (&RC.eNB[eNB_id][0]->ulsch_rate_unmatching_stats);
    reset_meas (&RC.eNB[eNB_id][0]->ulsch_turbo_decoding_stats);
    reset_meas (&RC.eNB[eNB_id][0]->ulsch_deinterleaving_stats);
    reset_meas (&RC.eNB[eNB_id][0]->ulsch_demultiplexing_stats);
    reset_meas (&RC.eNB[eNB_id][0]->ulsch_llr_stats);
    reset_meas (&RC.eNB[eNB_id][0]->ulsch_tc_init_stats);
    reset_meas (&RC.eNB[eNB_id][0]->ulsch_tc_alpha_stats);
    reset_meas (&RC.eNB[eNB_id][0]->ulsch_tc_beta_stats);
    reset_meas (&RC.eNB[eNB_id][0]->ulsch_tc_gamma_stats);
    reset_meas (&RC.eNB[eNB_id][0]->ulsch_tc_ext_stats);
    reset_meas (&RC.eNB[eNB_id][0]->ulsch_tc_intl1_stats);
    reset_meas (&RC.eNB[eNB_id][0]->ulsch_tc_intl2_stats);
#ifdef LOCALIZATION
    reset_meas(&RC.eNB[eNB_id][0]->localization_stats);
#endif

    /*
     * L2 functions
     */
    // eNB MAC
    reset_meas (&RC.mac[eNB_id]->eNB_scheduler); // total
    reset_meas (&RC.mac[eNB_id]->schedule_si); // only schedule + tx
    reset_meas (&RC.mac[eNB_id]->schedule_ra); // only ra
    reset_meas (&RC.mac[eNB_id]->schedule_ulsch); // onlu ulsch
    reset_meas (&RC.mac[eNB_id]->fill_DLSCH_dci); // only dci
    reset_meas (&RC.mac[eNB_id]->schedule_dlsch_preprocessor); // include rlc_data_req + MAC header gen
    reset_meas (&RC.mac[eNB_id]->schedule_dlsch); // include rlc_data_req + MAC header gen + pre-processor
    reset_meas (&RC.mac[eNB_id]->schedule_mch); // only embms
    reset_meas (&RC.mac[eNB_id]->rx_ulsch_sdu); // include rlc_data_ind + mac header parser

    reset_meas (&eNB_pdcp_stats[eNB_id].pdcp_run);
    reset_meas (&eNB_pdcp_stats[eNB_id].data_req);
    reset_meas (&eNB_pdcp_stats[eNB_id].data_ind);
    reset_meas (&eNB_pdcp_stats[eNB_id].apply_security);
    reset_meas (&eNB_pdcp_stats[eNB_id].validate_security);
    reset_meas (&eNB_pdcp_stats[eNB_id].pdcp_ip);
    reset_meas (&eNB_pdcp_stats[eNB_id].ip_pdcp);

  }
}

void
print_opp_meas_oaisim (void)
{

  uint8_t eNB_id = 0, UE_id = 0;

  print_meas (&oaisim_stats, "[OAI][total_exec_time]", &oaisim_stats,
              &oaisim_stats);
  print_meas (&oaisim_stats_f, "[OAI][SF_exec_time]", &oaisim_stats,
              &oaisim_stats_f);

  print_meas (&dl_chan_stats, "[DL][chan_stats]", &oaisim_stats,
              &oaisim_stats_f);
  print_meas (&ul_chan_stats, "[UL][chan_stats]", &oaisim_stats,
              &oaisim_stats_f);

  for (UE_id = 0; UE_id < NB_UE_INST; UE_id++) {
    for (ru_id = 0; ru_id < NB_RU; ru_id++) {
      print_meas (&RU2UE[ru_id][UE_id][0]->random_channel,
                  "[DL][random_channel]", &oaisim_stats, &oaisim_stats_f);
      print_meas (&RU2UE[ru_id][UE_id][0]->interp_time,
                  "[DL][interp_time]", &oaisim_stats, &oaisim_stats_f);
      print_meas (&RU2UE[ru_id][UE_id][0]->interp_freq,
                  "[DL][interp_freq]", &oaisim_stats, &oaisim_stats_f);
      print_meas (&RU2UE[ru_id][UE_id][0]->convolution,
                  "[DL][convolution]", &oaisim_stats, &oaisim_stats_f);

      print_meas (&UE2RU[UE_id][ru_id][0]->random_channel,
                  "[UL][random_channel]", &oaisim_stats, &oaisim_stats_f);
      print_meas (&UE2RU[UE_id][ru_id][0]->interp_time,
                  "[UL][interp_time]", &oaisim_stats, &oaisim_stats_f);
      print_meas (&UE2RU[UE_id][ru_id][0]->interp_freq,
                  "[UL][interp_freq]", &oaisim_stats, &oaisim_stats_f);
      print_meas (&UE2RU[UE_id][ru_id][0]->convolution,
                  "[UL][convolution]", &oaisim_stats, &oaisim_stats_f);
    }
  }

  for (UE_id = 0; UE_id < NB_UE_INST; UE_id++) {
    print_meas (&PHY_vars_UE_g[UE_id][0]->phy_proc[0], "[UE][total_phy_proc[0]]",
                &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->phy_proc[1], "[UE][total_phy_proc[1]]",
                &oaisim_stats, &oaisim_stats_f);


    print_meas (&PHY_vars_UE_g[UE_id][0]->phy_proc_rx[0],
                "[UE][total_phy_proc_rx[0]]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->phy_proc_rx[1],
                "[UE][total_phy_proc_rx[1]]", &oaisim_stats, &oaisim_stats_f);
    //    print_meas (&PHY_vars_UE_g[UE_id][0]->ofdm_demod_stats,
    //                "[UE][ofdm_demod]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->rx_dft_stats, "[UE][rx_dft]",
                &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_channel_estimation_stats,
                "[UE][channel_est]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_freq_offset_estimation_stats,
                "[UE][freq_offset]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_llr_stats, "[UE][llr]",
                &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_unscrambling_stats,
                "[UE][unscrambling]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_decoding_stats[0],
                "[UE][decoding[0]]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_decoding_stats[1],
                "[UE][decoding[1]]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_rate_unmatching_stats,
                "[UE][rate_unmatching]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_deinterleaving_stats,
                "[UE][deinterleaving]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_turbo_decoding_stats,
                "[UE][turbo_decoding]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_tc_init_stats,
                "[UE][ |_tc_init]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_tc_alpha_stats,
                "[UE][ |_tc_alpha]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_tc_beta_stats,
                "[UE][ |_tc_beta]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_tc_gamma_stats,
                "[UE][ |_tc_gamma]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_tc_ext_stats,
                "[UE][ |_tc_ext]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_tc_intl1_stats,
                "[UE][ |_tc_intl1]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->dlsch_tc_intl2_stats,
                "[UE][ |_tc_intl2]", &oaisim_stats, &oaisim_stats_f);

    print_meas (&PHY_vars_UE_g[UE_id][0]->phy_proc_tx,
                "[UE][total_phy_proc_tx]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->ofdm_mod_stats, "[UE][ofdm_mod]",
                &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->ulsch_modulation_stats,
                "[UE][modulation]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->ulsch_encoding_stats,
                "[UE][encoding]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->ulsch_segmentation_stats,
                "[UE][segmentation]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->ulsch_rate_matching_stats,
                "[UE][rate_matching]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->ulsch_turbo_encoding_stats,
                "[UE][turbo_encoding]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->ulsch_interleaving_stats,
                "[UE][interleaving]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&PHY_vars_UE_g[UE_id][0]->ulsch_multiplexing_stats,
                "[UE][multiplexing]", &oaisim_stats, &oaisim_stats_f);

  }

  for (eNB_id = 0; eNB_id < NB_eNB_INST; eNB_id++) {
    print_meas (&RC.eNB[eNB_id][0]->phy_proc,
                "[eNB][total_phy_proc]", &oaisim_stats, &oaisim_stats_f);

    print_meas (&RC.eNB[eNB_id][0]->phy_proc_tx,
                "[eNB][total_phy_proc_tx]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.eNB[eNB_id][0]->ofdm_mod_stats,
                "[eNB][ofdm_mod]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.eNB[eNB_id][0]->dlsch_modulation_stats,
                "[eNB][modulation]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.eNB[eNB_id][0]->dlsch_scrambling_stats,
                "[eNB][scrambling]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.eNB[eNB_id][0]->dlsch_encoding_stats,
                "[eNB][encoding]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.eNB[eNB_id][0]->dlsch_interleaving_stats,
                "[eNB][|_interleaving]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.eNB[eNB_id][0]->dlsch_rate_matching_stats,
                "[eNB][|_rate_matching]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.eNB[eNB_id][0]->dlsch_turbo_encoding_stats,
                "[eNB][|_turbo_encoding]", &oaisim_stats, &oaisim_stats_f);

    print_meas (&RC.eNB[eNB_id][0]->phy_proc_rx,
                "[eNB][total_phy_proc_rx]", &oaisim_stats, &oaisim_stats_f);
    //    print_meas (&RC.eNB[eNB_id][0]->ofdm_demod_stats,
    //                "[eNB][ofdm_demod]", &oaisim_stats, &oaisim_stats_f);
    //print_meas(&RC.eNB[eNB_id][0]->ulsch_channel_estimation_stats,"[eNB][channel_est]");
    //print_meas(&RC.eNB[eNB_id][0]->ulsch_freq_offset_estimation_stats,"[eNB][freq_offset]");
    //print_meas(&RC.eNB[eNB_id][0]->rx_dft_stats,"[eNB][rx_dft]");
    print_meas (&RC.eNB[eNB_id][0]->ulsch_demodulation_stats,
                "[eNB][demodulation]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.eNB[eNB_id][0]->ulsch_decoding_stats,
                "[eNB][decoding]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.eNB[eNB_id][0]->ulsch_deinterleaving_stats,
                "[eNB][|_deinterleaving]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.eNB[eNB_id][0]->ulsch_demultiplexing_stats,
                "[eNB][|_demultiplexing]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.eNB[eNB_id][0]->ulsch_rate_unmatching_stats,
                "[eNB][|_rate_unmatching]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.eNB[eNB_id][0]->ulsch_turbo_decoding_stats,
                "[eNB][|_turbo_decoding]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.eNB[eNB_id][0]->ulsch_tc_init_stats,
                "[eNB][ |_tc_init]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.eNB[eNB_id][0]->ulsch_tc_alpha_stats,
                "[eNB][ |_tc_alpha]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.eNB[eNB_id][0]->ulsch_tc_beta_stats,
                "[eNB][ |_tc_beta]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.eNB[eNB_id][0]->ulsch_tc_gamma_stats,
                "[eNB][ |_tc_gamma]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.eNB[eNB_id][0]->ulsch_tc_ext_stats,
                "[eNB][ |_tc_ext]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.eNB[eNB_id][0]->ulsch_tc_intl1_stats,
                "[eNB][ |_tc_intl1]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.eNB[eNB_id][0]->ulsch_tc_intl2_stats,
                "[eNB][ |_tc_intl2]", &oaisim_stats, &oaisim_stats_f);

    print_meas (&RC.eNB[eNB_id][0]->rx_prach, "[eNB][rx_prach]",
                &oaisim_stats, &oaisim_stats_f);

#ifdef LOCALIZATION
    print_meas(&RC.eNB[eNB_id][0]->localization_stats, "[eNB][LOCALIZATION]",&oaisim_stats,&oaisim_stats_f);
#endif
  }

  for (UE_id = 0; UE_id < NB_UE_INST; UE_id++) {

    print_meas (&UE_mac_inst[UE_id].ue_scheduler, "[UE][mac_scheduler]",
                &oaisim_stats, &oaisim_stats_f);
    print_meas (&UE_mac_inst[UE_id].tx_ulsch_sdu, "[UE][tx_ulsch_sdu]",
                &oaisim_stats, &oaisim_stats_f);
    print_meas (&UE_mac_inst[UE_id].rx_dlsch_sdu, "[UE][rx_dlsch_sdu]",
                &oaisim_stats, &oaisim_stats_f);
    print_meas (&UE_mac_inst[UE_id].ue_query_mch, "[UE][query_MCH]",
                &oaisim_stats, &oaisim_stats_f);
    print_meas (&UE_mac_inst[UE_id].rx_mch_sdu, "[UE][rx_mch_sdu]",
                &oaisim_stats, &oaisim_stats_f);
    print_meas (&UE_mac_inst[UE_id].rx_si, "[UE][rx_si]", &oaisim_stats,
                &oaisim_stats_f);

    print_meas (&UE_pdcp_stats[UE_id].pdcp_run, "[UE][total_pdcp_run]",
                &oaisim_stats, &oaisim_stats_f);
    print_meas (&UE_pdcp_stats[UE_id].data_req, "[UE][DL][pdcp_data_req]",
                &oaisim_stats, &oaisim_stats_f);
    print_meas (&UE_pdcp_stats[UE_id].data_ind, "[UE][UL][pdcp_data_ind]",
                &oaisim_stats, &oaisim_stats_f);

    print_meas (&UE_pdcp_stats[UE_id].apply_security,
                "[UE][DL][apply_security]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&UE_pdcp_stats[UE_id].validate_security,
                "[UE][UL][validate_security]", &oaisim_stats,
                &oaisim_stats_f);
    print_meas (&UE_pdcp_stats[UE_id].ip_pdcp, "[UE][DL][ip_pdcp]",
                &oaisim_stats, &oaisim_stats_f);
    print_meas (&UE_pdcp_stats[UE_id].pdcp_ip, "[UE][UL][pdcp_ip]",
                &oaisim_stats, &oaisim_stats_f);

  }

  for (eNB_id = 0; eNB_id < NB_eNB_INST; eNB_id++) {

    print_meas (&RC.mac[eNB_id]->eNB_scheduler, "[eNB][mac_scheduler]",
                &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.mac[eNB_id]->schedule_si, "[eNB][DL][SI]",
                &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.mac[eNB_id]->schedule_ra, "[eNB][DL][RA]",
                &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.mac[eNB_id]->fill_DLSCH_dci,
                "[eNB][DL/UL][fill_DCI]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.mac[eNB_id]->schedule_dlsch_preprocessor,
                "[eNB][DL][preprocessor]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.mac[eNB_id]->schedule_dlsch,
                "[eNB][DL][schedule_tx_dlsch]", &oaisim_stats,
                &oaisim_stats_f);
    print_meas (&RC.mac[eNB_id]->schedule_mch, "[eNB][DL][mch]",
                &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.mac[eNB_id]->schedule_ulsch, "[eNB][UL][ULSCH]",
                &oaisim_stats, &oaisim_stats_f);
    print_meas (&RC.mac[eNB_id]->rx_ulsch_sdu,
                "[eNB][UL][rx_ulsch_sdu]", &oaisim_stats, &oaisim_stats_f);

    print_meas (&eNB_pdcp_stats[eNB_id].pdcp_run, "[eNB][pdcp_run]",
                &oaisim_stats, &oaisim_stats_f);
    print_meas (&eNB_pdcp_stats[eNB_id].data_req,
                "[eNB][DL][pdcp_data_req]", &oaisim_stats, &oaisim_stats_f);
    print_meas (&eNB_pdcp_stats[eNB_id].data_ind,
                "[eNB][UL][pdcp_data_ind]", &oaisim_stats, &oaisim_stats_f);

    print_meas (&eNB_pdcp_stats[eNB_id].apply_security,
                "[eNB][DL][apply_security]", &oaisim_stats,
                &oaisim_stats_f);
    print_meas (&eNB_pdcp_stats[eNB_id].validate_security,
                "[eNB][UL][validate_security]", &oaisim_stats,
                &oaisim_stats_f);
    print_meas (&eNB_pdcp_stats[eNB_id].ip_pdcp, "[eNB][DL][ip_pdcp]",
                &oaisim_stats, &oaisim_stats_f);
    print_meas (&eNB_pdcp_stats[eNB_id].pdcp_ip, "[eNB][UL][pdcp_ip]",
                &oaisim_stats, &oaisim_stats_f);

  }

}

#if !defined(ENABLE_ITTI)
static void *
sigh (void *arg)
{

  int signum;
  sigset_t sigcatch;
  sigemptyset (&sigcatch);
  sigaddset (&sigcatch, SIGHUP);
  sigaddset (&sigcatch, SIGINT);
  sigaddset (&sigcatch, SIGTERM);
  sigaddset (&sigcatch, SIGQUIT);

  for (;;) {
    sigwait (&sigcatch, &signum);

    //sigwait(&sigblock, &signum);
    switch (signum) {
    case SIGHUP:
    case SIGINT:
    case SIGTERM:
    case SIGQUIT:
      fprintf (stderr, "received signal %d \n", signum);
      // no need for mutx: when ITTI not used, this variable is only accessed by this function
      l2l1_state = L2L1_TERMINATED;
      break;

    default:
      fprintf (stderr, "Unexpected signal %d \n", signum);
      exit (-1);
      break;
    }
  }

  pthread_exit (NULL);
}
#endif /* !defined(ENABLE_ITTI) */

void
oai_shutdown (void)
{
  static int done = 0;

  if (done)
    return;

  free (otg_pdcp_buffer);
  otg_pdcp_buffer = 0;

#ifdef SMBV

  // Rohde&Schwarz SMBV100A vector signal generator
  if (config_smbv) {
    smbv_send_config (smbv_fname,smbv_ip);
  }

#endif

  //Perform KPI measurements
  if (oai_emulation.info.otg_enabled == 1){
    LOG_N(EMU,"calling OTG kpi gen .... \n");
    kpi_gen ();
  }
  if (oai_emulation.info.opp_enabled == 1)
    print_opp_meas_oaisim ();


#ifdef PROC

  if (abstraction_flag == 0 && Channel_Flag==0 && Process_Flag==0)
#else
    if (abstraction_flag == 0)
#endif
      {
	/*
	  #ifdef IFFT_FPGA
	  free(txdataF2[0]);
	  free(txdataF2[1]);
	  free(txdataF2);
	  free(txdata[0]);
	  free(txdata[1]);
	  free(txdata);
	  #endif
	*/
	/*
	for (int i = 0; i < 2; i++) {
	  free (s_re[i]);
	  free (s_im[i]);
	  free (r_re[i]);
	  free (r_im[i]);
	}

	free (s_re);
	free (s_im);
	free (r_re);
	free (r_im);
	s_re = 0;
	s_im = 0;
	r_re = 0;
	r_im = 0;*/

	lte_sync_time_free ();
      }

  // added for PHY abstraction
  if (oai_emulation.info.ocm_enabled == 1) {
    for (eNB_inst = 0; eNB_inst < NUMBER_OF_eNB_MAX; eNB_inst++) {
      free (enb_data[eNB_inst]);
      enb_data[eNB_inst] = 0;
    }

    for (UE_inst = 0; UE_inst < NUMBER_OF_UE_MAX; UE_inst++) {
      free (ue_data[UE_inst]);
      ue_data[UE_inst] = 0;
    }
  } //End of PHY abstraction changes


  // stop OMG
  stop_mobility_generator (omg_param_list); //omg_param_list.mobility_type
#ifdef OPENAIR2

  if (oai_emulation.info.omv_enabled == 1)
    omv_end (pfd[1], omv_data);

#endif

  if ((oai_emulation.info.ocm_enabled == 1) && (ethernet_flag == 0)
      && (ShaF != NULL)) {
    destroyMat (ShaF, map1, map2);
    ShaF = 0;
  }

  if (opt_enabled == 1)
    terminate_opt ();

  if (oai_emulation.info.cli_enabled)
    cli_server_cleanup ();

  for (int i = 0; i < NUMBER_OF_eNB_MAX + NUMBER_OF_UE_MAX; i++)
    if (oai_emulation.info.oai_ifup[i] == 1) {
      char interfaceName[8];
      snprintf (interfaceName, sizeof(interfaceName), "oai%d", i);
      bringInterfaceUp (interfaceName, 0);
    }

  log_thread_finalize ();
  logClean ();
  VCD_SIGNAL_DUMPER_CLOSE ();

  done = 1; // prevent next invokation of this function

  LOG_N(EMU,
        ">>>>>>>>>>>>>>>>>>>>>>>>>>> OAIEMU shutdown <<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");
}

eNB_MAC_INST*
get_eNB_mac_inst (module_id_t module_idP)
{
  return (RC.mac[module_idP]);
}

OAI_Emulation*
get_OAI_emulation ()
{
  return &oai_emulation;
}


// dummy function declarations

void *rrc_enb_task(void *args_p)
{
  return NULL;
}

