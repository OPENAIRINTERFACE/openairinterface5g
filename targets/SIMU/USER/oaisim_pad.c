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

#include <string.h>
#include <math.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cblas.h>
#include <execinfo.h>

//<<PAD>>//
//#include <mpi.h>
//#include "UTIL/FIFO/pad_list.h"
#include "discrete_event_generator.h"
#include "threadpool.h"
#include <pthread.h>
#include "oaisim_functions.h"
//<<PAD>>//

#include "SIMULATION/RF/defs.h"
#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/vars.h"
#include "MAC_INTERFACE/vars.h"

//#ifdef OPENAIR2
#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/vars.h"
#include "RRC/LITE/vars.h"
#include "PHY_INTERFACE/vars.h"
//#endif

#include "ARCH/CBMIMO1/DEVICE_DRIVER/vars.h"

#ifdef IFFT_FPGA
//#include "PHY/LTE_REFSIG/mod_table.h"
#endif //IFFT_FPGA

#include "SCHED/defs.h"
#include "SCHED/vars.h"

#include "oaisim.h"
#include "oaisim_config.h"
#include "UTIL/OCG/OCG_extern.h"
#include "cor_SF_sim.h"
#include "UTIL/OMG/omg_constants.h"



//#include "UTIL/LOG/vcd_signal_dumper.h"

#define RF

//#define DEBUG_SIM

#define MCS_COUNT 24//added for PHY abstraction
#define N_TRIALS 1


/*
  DCI0_5MHz_TDD0_t          UL_alloc_pdu;
  DCI1A_5MHz_TDD_1_6_t      CCCH_alloc_pdu;
  DCI2_5MHz_2A_L10PRB_TDD_t DLSCH_alloc_pdu1;
  DCI2_5MHz_2A_M10PRB_TDD_t DLSCH_alloc_pdu2;
 */

#define UL_RB_ALLOC computeRIV(lte_frame_parms->N_RB_UL,0,24)
#define CCCH_RB_ALLOC computeRIV(lte_frame_parms->N_RB_UL,0,3)
#define RA_RB_ALLOC computeRIV(lte_frame_parms->N_RB_UL,0,3)
#define DLSCH_RB_ALLOC 0x1fff

#define DECOR_DIST 100
#define SF_VAR 10

//constant for OAISIM soft realtime calibration
#define SF_DEVIATION_OFFSET_NS 100000 //= 0.1ms : should be as a number of UE
#define SLEEP_STEP_US       100 //  = 0.01ms could be adaptive, should be as a number of UE
#define K 2                  // averaging coefficient
#define TARGET_SF_TIME_NS 1000000       // 1ms = 1000000 ns

//#ifdef OPENAIR2
//uint16_t NODE_ID[1];
//uint8_t NB_INST = 2;
//#endif //OPENAIR2
extern int otg_times;
extern int for_times;
extern int if_times;
int for_main_times = 0;

frame_t frame=0;
char stats_buffer[16384];
channel_desc_t *eNB2UE[NUMBER_OF_eNB_MAX][NUMBER_OF_UE_MAX];
channel_desc_t *UE2eNB[NUMBER_OF_UE_MAX][NUMBER_OF_eNB_MAX];
Signal_buffers *signal_buffers_g;
//Added for PHY abstraction
node_desc_t *enb_data[NUMBER_OF_eNB_MAX];
node_desc_t *ue_data[NUMBER_OF_UE_MAX];
//double sinr_bler_map[MCS_COUNT][2][16];
//double sinr_bler_map_up[MCS_COUNT][2][16];
//extern double SINRpost_eff[301];
extern int mcsPost;
extern int  nrbPost;
extern int frbPost;
extern void kpi_gen();

extern uint16_t Nid_cell;
extern uint8_t target_dl_mcs;
extern uint8_t rate_adaptation_flag;
extern double snr_dB, sinr_dB;
extern uint8_t set_seed;
extern uint8_t cooperation_flag;          // for cooperative communication
extern uint8_t abstraction_flag, ethernet_flag;
extern uint8_t ue_connection_test;
extern int map1,map2;
extern double **ShaF;
// pointers signal buffers (s = transmit, r,r0 = receive)
extern double **s_re, **s_im, **r_re, **r_im, **r_re0, **r_im0;
extern Node_list ue_node_list;
extern Node_list enb_node_list;
extern int pdcp_period, omg_period;
extern LTE_DL_FRAME_PARMS *frame_parms;
// time calibration for soft realtime mode
extern struct timespec time_spec;
extern unsigned long time_last, time_now;
extern int td, td_avg, sleep_time_us;

int eMBMS_active = 0;

threadpool_t * pool;

#ifdef OPENAIR2
extern int pfd[2];
#endif

// this should reflect the channel models in openair1/SIMULATION/TOOLS/defs.h
mapping small_scale_names[] = {
  {"custom", custom},
  {"SCM_A", SCM_A},
  {"SCM_B", SCM_B},
  {"SCM_C", SCM_C},
  {"SCM_D", SCM_D},
  {"EPA", EPA},
  {"EVA", EVA},
  {"ETU", ETU},
  {"Rayleigh8", Rayleigh8},
  {"Rayleigh1", Rayleigh1},
  {"Rayleigh1_800", Rayleigh1_800},
  {"Rayleigh1_corr", Rayleigh1_corr},
  {"Rayleigh1_anticorr", Rayleigh1_anticorr},
  {"Rice8", Rice8},
  {"Rice1", Rice1},
  {"Rice1_corr", Rice1_corr},
  {"Rice1_anticorr", Rice1_anticorr},
  {"AWGN", AWGN},
  {NULL, -1}
};

//static void *sigh(void *arg);
void terminate(void);

void
help (void)
{
  printf
  ("Usage: oaisim -h -a -F -C tdd_config -V -R N_RB_DL -e -x transmission_mode -m target_dl_mcs -r(ate_adaptation) -n n_frames -s snr_dB -k ricean_factor -t max_delay -f forgetting factor -A channel_model -z cooperation_flag -u nb_local_ue -U UE mobility -b nb_local_enb -B eNB_mobility -M ethernet_flag -p nb_master -g multicast_group -l log_level -c ocg_enable -T traffic model -D multicast network device\n");

  printf ("-h provides this help message!\n");
  printf ("-a Activates PHY abstraction mode\n");
  printf ("-F Activates FDD transmission (TDD is default)\n");
  printf ("-C [0-6] Sets TDD configuration\n");
  printf ("-R [6,15,25,50,75,100] Sets N_RB_DL\n");
  printf ("-e Activates extended prefix mode\n");
  printf ("-m Gives a fixed DL mcs\n");
  printf ("-r Activates rate adaptation (DL for now)\n");
  printf ("-n Set the number of frames for the simulation\n");
  printf ("-s snr_dB set a fixed (average) SNR, this deactivates the openair channel model generator (OCM)\n");
  printf ("-S snir_dB set a fixed (average) SNIR, this deactivates the openair channel model generator (OCM)\n");
  printf ("-k Set the Ricean factor (linear)\n");
  printf ("-t Set the delay spread (microseconds)\n");
  printf ("-f Set the forgetting factor for time-variation\n");
  printf ("-A set the multipath channel simulation,  options are: SCM_A, SCM_B, SCM_C, SCM_D, EPA, EVA, ETU, Rayleigh8, Rayleigh1, Rayleigh1_corr,Rayleigh1_anticorr, Rice8,, Rice1, AWGN \n");
  printf ("-b Set the number of local eNB\n");
  printf ("-u Set the number of local UE\n");
  printf ("-M Set the machine ID for Ethernet-based emulation\n");
  printf ("-p Set the total number of machine in emulation - valid if M is set\n");
  printf ("-g Set multicast group ID (0,1,2,3) - valid if M is set\n");
  printf ("-l Set the global log level (8:trace, 7:debug, 6:info, 4:warn, 3:error) \n");
  printf
  ("-c [1,2,3,4] Activate the config generator (OCG) to process the scenario descriptor, or give the scenario manually: -c template_1.xml \n");
  printf ("-x Set the transmission mode (1,2,5,6 supported for now)\n");
  printf ("-z Set the cooperation flag (0 for no cooperation, 1 for delay diversity and 2 for distributed alamouti\n");
  printf ("-T activate the traffic generator: 0 for NONE, 1 for CBR, 2 for M2M, 3 for FPS Gaming, 4 for mix\n");
  printf ("-B Set the mobility model for eNB, options are: STATIC, RWP, RWALK, \n");
  printf ("-U Set the mobility model for UE, options are: STATIC, RWP, RWALK \n");
  printf ("-E Random number generator seed\n");
  printf ("-P enable protocol analyzer : 0 for wireshark interface, 1: for pcap , 2 : for tshark \n");
  printf ("-I Enable CLI interface (to connect use telnet localhost 1352)\n");
  printf ("-V Enable VCD dump, file = openair_vcd_dump.vcd\n");
  printf ("-G Enable background traffic \n");
  printf ("-O [mme ipv4 address] Enable MME mode\n");
  printf ("-Z Reserved\n");
}


#ifdef OPENAIR2
void omv_end (int pfd, Data_Flow_Unit omv_data);
int omv_write (int pfd,  Node_list enb_node_list, Node_list ue_node_list, Data_Flow_Unit omv_data);
#endif

//<<<< PAD >>>>//
#define PAD 1
//#define PAD_FINE 1
//#define PAD_SYNC 1
#define JOB_REQUEST_TAG 246
#define JOB_REPLY_TAG 369
#define FRAME_END 888
#define NO_JOBS_TAG 404
#define JOB_DIST_DEBUG 33

//Global Variables
int worker_number;
int frame_number = 1;
//<<<< PAD >>>>//

//<<<< DEG >>>>//
extern End_Of_Sim_Event end_event; //Could later be a list of condition_events
extern Event_List event_list;
//<<<< DEG >>>>//

extern Packet_OTG_List *otg_pdcp_buffer;

void run(int argc, char *argv[]);

#ifdef PAD
void pad_init()
{

  int UE_id, i;

  pool = threadpool_create(PAD);

  if (pool == NULL) {
    printf("ERROR threadpool allocation\n");
    return;
  }

  signal_buffers_g = malloc(NB_UE_INST * sizeof(Signal_buffers));

  if (abstraction_flag == 0) {
    for (UE_id = 0; UE_id < NB_UE_INST; UE_id++) {
      signal_buffers_g[UE_id].s_re = malloc(2*sizeof(double*));
      signal_buffers_g[UE_id].s_im = malloc(2*sizeof(double*));
      signal_buffers_g[UE_id].r_re = malloc(2*sizeof(double*));
      signal_buffers_g[UE_id].r_im = malloc(2*sizeof(double*));
      signal_buffers_g[UE_id].r_re0 = malloc(2*sizeof(double*));
      signal_buffers_g[UE_id].r_im0 = malloc(2*sizeof(double*));


      for (i=0; i<2; i++) {

        signal_buffers_g[UE_id].s_re[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
        bzero(signal_buffers_g[UE_id].s_re[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
        signal_buffers_g[UE_id].s_im[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
        bzero(signal_buffers_g[UE_id].s_im[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
        signal_buffers_g[UE_id].r_re[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
        bzero(signal_buffers_g[UE_id].r_re[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
        signal_buffers_g[UE_id].r_im[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
        bzero(signal_buffers_g[UE_id].r_im[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
        signal_buffers_g[UE_id].r_re0[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
        bzero(signal_buffers_g[UE_id].r_re0[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
        signal_buffers_g[UE_id].r_im0[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
        bzero(signal_buffers_g[UE_id].r_im0[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
      }
    }
  }
}

void pad_finalize()
{

  int ret, i;
  module_id_t UE_id;

  ret = threadpool_destroy(pool);

  if (ret)
    printf("ERROR threadpool destroy = %d\n", ret);

  if (abstraction_flag == 0) {

    for (UE_id = 0; UE_id < NB_UE_INST; UE_id++) {
      for (i = 0; i < 2; i++) {
        free(signal_buffers_g[UE_id].s_re[i]);
        free(signal_buffers_g[UE_id].s_im[i]);
        free(signal_buffers_g[UE_id].r_re[i]);
        free(signal_buffers_g[UE_id].r_im[i]);
      }

      free(signal_buffers_g[UE_id].s_re);
      free(signal_buffers_g[UE_id].s_im);
      free(signal_buffers_g[UE_id].r_re);
      free(signal_buffers_g[UE_id].r_im);
    }

    //free node by node here same pattern as below
  }

  free(signal_buffers_g);
}

void pad_inject_job(int eNB_flag, int nid, int frame, int next_slot, int last_slot, enum Job_type type, int ctime)
{

  int ret;
  Job_elt *job_elt;

  job_elt = malloc(sizeof(Job_elt));
  job_elt->next = NULL;
  (job_elt->job).eNB_flag = eNB_flag;
  (job_elt->job).nid = nid;
  (job_elt->job).frame = frame;
  (job_elt->job).next_slot = next_slot;
  (job_elt->job).last_slot = last_slot;
  (job_elt->job).type = type;
  (job_elt->job).ctime = ctime;

  ret = threadpool_add(pool, job_elt);

  if (ret) {
    printf("ERROR threadpool_add %d\n", ret);
    return;
  }
}

void pad_synchronize()
{
  pthread_mutex_lock(&(pool->sync_lock));

  while(pool->active > 0) {
    pthread_cond_wait(&(pool->sync_notify), &(pool->sync_lock));
  }

  pthread_mutex_unlock(&(pool->sync_lock));
}

#endif
//<<PAD(DEG_MAIN)>>//
int main (int argc, char *argv[])
{

  //Mobility *mobility_frame_10;
  //Application_Config *application_frame_20;

  //Here make modifications on the mobility and traffic new models
  //mob_frame_10 -> ...
  //application_frame_30 -> ...

  //schedule(ET_OMG, 10, NULL, mobility_frame_10);
  //schedule(ET_OTG, 15, NULL, application_frame_20);

  //event_list_display(&event_list);

  schedule_end_of_simulation(FRAME, 100);

  run(argc, argv);

  return 0;
}
//<<PAD>>//

//<<PAD(RUN)>>//
void run(int argc, char *argv[])
{


  int32_t i;
  module_id_t UE_id, eNB_id;
  Job_elt *job_elt;
  int ret;

  clock_t t;


  Event_elt *user_defined_event;
  Event event;

  // Framing variables
  int32_t slot, last_slot, next_slot;

  FILE *SINRpost;
  char SINRpost_fname[512];
  sprintf(SINRpost_fname,"postprocSINR.m");
  SINRpost = fopen(SINRpost_fname,"w");
  // variables/flags which are set by user on command-line
  double snr_direction,snr_step=1.0;//,sinr_direction;

  lte_subframe_t direction;
  char fname[64],vname[64];

#ifdef XFORMS
  // current status is that every UE has a DL scope for a SINGLE eNB (eNB_id=0)
  // at eNB 0, an UL scope for every UE
  FD_lte_phy_scope_ue  *form_ue[NUMBER_OF_UE_MAX];
  FD_lte_phy_scope_enb *form_enb[NUMBER_OF_UE_MAX];
  char title[255];
#endif

#ifdef PROC
  int node_id;
  int port,Process_Flag=0,wgt,Channel_Flag=0,temp;
#endif

  // uint8_t awgn_flag = 0;

#ifdef PRINT_STATS
  int len;
  FILE *UE_stats[NUMBER_OF_UE_MAX], *UE_stats_th[NUMBER_OF_UE_MAX], *eNB_stats, *eNB_avg_thr, *eNB_l2_stats;
  char UE_stats_filename[255];
  char UE_stats_th_filename[255];
  char eNB_stats_th_filename[255];
#endif

#ifdef SMBV
  uint8_t config_smbv = 0;
  char smbv_ip[16];
  strcpy(smbv_ip,DEFAULT_SMBV_IP);
#endif

#ifdef OPENAIR2
  Data_Flow_Unit omv_data;
#endif


  //time_t t0,t1;
  //clock_t start, stop;

  //double **s_re2[MAX_eNB+MAX_UE], **s_im2[MAX_eNB+MAX_UE], **r_re2[MAX_eNB+MAX_UE], **r_im2[MAX_eNB+MAX_UE], **r_re02, **r_im02;
  //double **r_re0_d[MAX_UE][MAX_eNB], **r_im0_d[MAX_UE][MAX_eNB], **r_re0_u[MAX_eNB][MAX_UE],**r_im0_u[MAX_eNB][MAX_UE];
  //default parameters

  //{
  /* INITIALIZATIONS */

  target_dl_mcs = 0;
  rate_adaptation_flag = 0;
  oai_emulation.info.n_frames = 0xffff;//1024;          //10;
  oai_emulation.info.n_frames_flag = 0;//fixme
  snr_dB = 30;
  cooperation_flag = 0;         // default value 0 for no cooperation, 1 for Delay diversity, 2 for Distributed Alamouti

  //Default values if not changed by the user in get_simulation_options();
  pdcp_period = 1;
  omg_period = 10;

  mRAL_init_default_values(); //Default values
  eRAL_init_default_values(); //Default values

  init_oai_emulation(); //Default values

  get_simulation_options(argc, argv); //Command-line options

  oaisim_config(); // config OMG and OCG, OPT, OTG, OLG

  //To fix eventual conflict on the value of n_frames
  if (oai_emulation.info.n_frames_flag) {
    schedule_end_of_simulation(FRAME, oai_emulation.info.n_frames);
  }

  VCD_SIGNAL_DUMPER_INIT(); // Initialize VCD LOG module

#ifdef OPENAIR2
  init_omv();
#endif

  check_and_adjust_params(); //Before this call, NB_UE_INST and NB_eNB_INST are not set correctly

  init_otg_pdcp_buffer();

#ifdef PRINT_STATS

  for (UE_id=0; UE_id<NB_UE_INST; UE_id++) {
    sprintf(UE_stats_filename,"UE_stats%d.txt",UE_id);
    UE_stats[UE_id] = fopen (UE_stats_filename, "w");
  }

  eNB_stats = fopen ("eNB_stats.txt", "w");
  printf ("UE_stats=%p, eNB_stats=%p\n", UE_stats, eNB_stats);

  eNB_avg_thr = fopen ("eNB_stats_th.txt", "w");

#endif

  LOG_I(EMU,"total number of UE %d (local %d, remote %d) mobility %s \n", NB_UE_INST,oai_emulation.info.nb_ue_local,oai_emulation.info.nb_ue_remote,
        oai_emulation.topology_config.mobility.eNB_mobility.eNB_mobility_type.selected_option);
  LOG_I(EMU,"Total number of eNB %d (local %d, remote %d) mobility %s \n", NB_eNB_INST,oai_emulation.info.nb_enb_local,oai_emulation.info.nb_enb_remote,
        oai_emulation.topology_config.mobility.UE_mobility.UE_mobility_type.selected_option);
  LOG_I(OCM,"Running with frame_type %d, Nid_cell %d, N_RB_DL %d, EP %d, mode %d, target dl_mcs %d, rate adaptation %d, nframes %d, abstraction %d, channel %s\n",
        oai_emulation.info.frame_type, Nid_cell, oai_emulation.info.N_RB_DL, oai_emulation.info.extended_prefix_flag, oai_emulation.info.transmission_mode,target_dl_mcs,rate_adaptation_flag,
        oai_emulation.info.n_frames,abstraction_flag,oai_emulation.environment_system_config.fading.small_scale.selected_option);

  init_seed(set_seed);

  init_openair1();

  init_openair2();

  init_ocm();

#ifdef XFORMS
  init_xforms();
#endif

  printf ("before L2 init: Nid_cell %d\n", PHY_vars_eNB_g[0]->lte_frame_parms.Nid_cell);
  printf ("before L2 init: frame_type %d,tdd_config %d\n",
          PHY_vars_eNB_g[0]->lte_frame_parms.frame_type,
          PHY_vars_eNB_g[0]->lte_frame_parms.tdd_config);

  init_time();

#ifdef PAD
  pad_init();
#endif

  if (ue_connection_test == 1) {
    snr_direction = -snr_step;
    snr_dB=20;
    sinr_dB=-20;
  }

  frame = 0;
  slot = 0;

  LOG_I(EMU,">>>>>>>>>>>>>>>>>>>>>>>>>>> OAIEMU initialization done <<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

  printf ("after init: Nid_cell %d\n", PHY_vars_eNB_g[0]->lte_frame_parms.Nid_cell);
  printf ("after init: frame_type %d,tdd_config %d\n",
          PHY_vars_eNB_g[0]->lte_frame_parms.frame_type,
          PHY_vars_eNB_g[0]->lte_frame_parms.tdd_config);

  t = clock();

  while (!end_of_simulation()) {

    last_slot = (slot - 1)%20;

    if (last_slot <0)
      last_slot+=20;

    next_slot = (slot + 1)%20;

    oai_emulation.info.time_ms = frame * 10 + (next_slot>>1);
    oai_emulation.info.frame = frame;

    if (slot == 0) { //Frame's Prologue

      //Run the aperiodic user-defined events
      while ((user_defined_event = event_list_get_head(&event_list)) != NULL) {

        event = user_defined_event->event;

        if (event.frame == frame) {
          switch (event.type) {
          case ET_OMG:
            update_omg_model(event.key, event.value); //implement it with assigning the new values to that of oai_emulation & second thing is to ensure mob model is always read from oai_emulation
            user_defined_event = event_list_remove_head(&event_list);
            break;

          case ET_OTG:
            update_otg_model(event.key, event.value);
            user_defined_event = event_list_remove_head(&event_list);
            break;
          }
        } else {
          break;
        }
      }

      //Comment (handle cooperation flag) deleted here. Look at oaisim.c to see it
      if (ue_connection_test==1) {
        if ((frame%20) == 0) {
          snr_dB += snr_direction;
          sinr_dB -= snr_direction;
        }

        if (snr_dB == -20) {
          snr_direction=snr_step;
        } else if (snr_dB==20) {
          snr_direction=-snr_step;
        }
      }

      update_omg(); // frequency is defined in the omg_global params configurable by the user

      update_omg_ocm();

#ifdef OPENAIR2

      // check if pipe is still open
      if ((oai_emulation.info.omv_enabled == 1) ) {
        omv_write(pfd[1], enb_node_list, ue_node_list, omv_data);
      }

#endif

#ifdef DEBUG_OMG

      if ((((int) oai_emulation.info.time_s) % 100) == 0) {
        for (UE_id = oai_emulation.info.first_ue_local; UE_id < (oai_emulation.info.first_ue_local + oai_emulation.info.nb_ue_local); UE_id++) {
          get_node_position (UE, UE_id);
        }
      }

#endif
      update_ocm();
    }

    direction = subframe_select(frame_parms,next_slot>>1);

    if((next_slot %2) ==0)
      clear_eNB_transport_info(oai_emulation.info.nb_enb_local);


    for (eNB_id=oai_emulation.info.first_enb_local;
         (eNB_id<(oai_emulation.info.first_enb_local+oai_emulation.info.nb_enb_local)) && (oai_emulation.info.cli_start_enb[eNB_id]==1);
         eNB_id++) {
      for_main_times += 1;
      //printf ("debug: Nid_cell %d\n", PHY_vars_eNB_g[eNB_id]->lte_frame_parms.Nid_cell);
      //printf ("debug: frame_type %d,tdd_config %d\n", PHY_vars_eNB_g[eNB_id]->lte_frame_parms.frame_type,PHY_vars_eNB_g[eNB_id]->lte_frame_parms.tdd_config);
      LOG_D(EMU,"PHY procedures eNB %d for frame %d, slot %d (subframe TX %d, RX %d) TDD %d/%d Nid_cell %d\n",
            eNB_id, frame, slot, next_slot >> 1,last_slot>>1,
            PHY_vars_eNB_g[eNB_id]->lte_frame_parms.frame_type,
            PHY_vars_eNB_g[eNB_id]->lte_frame_parms.tdd_config,PHY_vars_eNB_g[eNB_id]->lte_frame_parms.Nid_cell);

      //Appliation
#ifdef PAD_FINE
      pad_inject_job(1, eNB_id, frame, next_slot, last_slot, JT_OTG, oai_emulation.info.time_ms);
#else
      update_otg_eNB(eNB_id, oai_emulation.info.time_ms);
#endif

      //Access layer
      if (frame % pdcp_period == 0) {
#ifdef PAD_FINE
        pad_inject_job(1, eNB_id, frame, next_slot, last_slot, JT_PDCP, oai_emulation.info.time_ms);
#else
        pdcp_run(frame, 1, 0, eNB_id);//PHY_vars_eNB_g[eNB_id]->Mod_id
#endif
      }

      //Phy/Mac layer
#ifdef PAD_FINE
      pad_inject_job(1, eNB_id, frame, next_slot, last_slot, JT_PHY_MAC, oai_emulation.info.time_ms);
#else
      phy_procedures_eNB_lte (last_slot, next_slot, PHY_vars_eNB_g[eNB_id], abstraction_flag, no_relay, NULL);
#endif


#ifdef PRINT_STATS

      if(last_slot==9 && frame%10==0)
        if(eNB_avg_thr)
          fprintf(eNB_avg_thr,"%d %d\n",PHY_vars_eNB_g[eNB_id]->frame,(PHY_vars_eNB_g[eNB_id]->total_system_throughput)/((PHY_vars_eNB_g[eNB_id]->frame+1)*10));

      if (eNB_stats) {
        len = dump_eNB_stats(PHY_vars_eNB_g[eNB_id], stats_buffer, 0);
        rewind (eNB_stats);
        fwrite (stats_buffer, 1, len, eNB_stats);
        fflush(eNB_stats);
      }

#ifdef OPENAIR2

      if (eNB_l2_stats) {
        len = dump_eNB_l2_stats (stats_buffer, 0);
        rewind (eNB_l2_stats);
        fwrite (stats_buffer, 1, len, eNB_l2_stats);
        fflush(eNB_l2_stats);
      }

#endif
#endif
    }

#ifdef PAD_SYNC

    if ((direction == SF_DL) || ((direction == SF_S) && (next_slot%2==0)) )
      pad_synchronize();

#endif


    // Call ETHERNET emulation here
    //emu_transport (frame, last_slot, next_slot, direction, oai_emulation.info.frame_type, ethernet_flag);

    if ((next_slot % 2) == 0)
      clear_UE_transport_info (oai_emulation.info.nb_ue_local);

    for (UE_id = oai_emulation.info.first_ue_local;
         (UE_id < (oai_emulation.info.first_ue_local+oai_emulation.info.nb_ue_local)) && (oai_emulation.info.cli_start_ue[UE_id]==1);
         UE_id++)
      if (frame >= (UE_id * 20)) {    // activate UE only after 20*UE_id frames so that different UEs turn on separately

        LOG_D(EMU,"PHY procedures UE %d for frame %d, slot %d (subframe TX %d, RX %d)\n",
              UE_id, frame, slot, next_slot >> 1,last_slot>>1);

        if (PHY_vars_UE_g[UE_id]->UE_mode[0] != NOT_SYNCHED) {
          if (frame>0) {
            PHY_vars_UE_g[UE_id]->frame = frame;

            //Application UE
#ifdef PAD_FINE
            pad_inject_job(0, UE_id, frame, next_slot, last_slot, JT_OTG, oai_emulation.info.time_ms);
#else
            update_otg_UE(UE_id + NB_eNB_INST, oai_emulation.info.time_ms);
#endif

            //Access layer UE
            if (frame % pdcp_period == 0) {
#ifdef PAD_FINE
              pad_inject_job(0, UE_id, frame, next_slot, last_slot, JT_PDCP, oai_emulation.info.time_ms);
#else
              pdcp_run(frame, 0, UE_id, 0);
#endif
            }

            //Phy/Mac layer UE
#ifdef PAD_FINE
            pad_inject_job(0, UE_id, frame, next_slot, last_slot, JT_PHY_MAC, oai_emulation.info.time_ms);
#else
            phy_procedures_UE_lte (last_slot, next_slot, PHY_vars_UE_g[UE_id], 0, abstraction_flag, normal_txrx, no_relay, NULL);
            ue_data[UE_id]->tx_power_dBm = PHY_vars_UE_g[UE_id]->tx_power_dBm;
#endif
          }
        } else {
          if (abstraction_flag==1) {
            LOG_E(EMU, "sync not supported in abstraction mode (UE%d,mode%d)\n", UE_id, PHY_vars_UE_g[UE_id]->UE_mode[0]);
            exit(-1);
          }

          if ((frame>0) && (last_slot == (LTE_SLOTS_PER_FRAME-2))) {
#ifdef PAD_FINE
            pad_inject_job(0, UE_id, frame, next_slot, last_slot, JT_INIT_SYNC, oai_emulation.info.time_ms);
#else
            initial_sync(PHY_vars_UE_g[UE_id],normal_txrx);
#endif
            /* LONG write output comment DELETED here */
          }
        }

#ifdef PRINT_STATS

        if(last_slot==2 && frame%10==0)
          if (UE_stats_th[UE_id])
            fprintf(UE_stats_th[UE_id],"%d %d\n",frame, PHY_vars_UE_g[UE_id]->bitrate[0]/1000);

        if (UE_stats[UE_id]) {
          len = dump_ue_stats (PHY_vars_UE_g[UE_id], stats_buffer, 0, normal_txrx, 0);
          rewind (UE_stats[UE_id]);
          fwrite (stats_buffer, 1, len, UE_stats[UE_id]);
          fflush(UE_stats[UE_id]);
        }

#endif
      }

#ifdef PAD_SYNC

    if ((direction == SF_UL) || ((direction == SF_S) && (next_slot%2==1)) )
      pad_synchronize();

#endif

    emu_transport (frame, last_slot, next_slot,direction, oai_emulation.info.frame_type, ethernet_flag);

    if ((direction  == SF_DL)|| (frame_parms->frame_type==0)) {
      for (UE_id=0; UE_id<NB_UE_INST; UE_id++) {

#ifdef PAD
        pad_inject_job(0, UE_id, frame, next_slot, last_slot, JT_DL, oai_emulation.info.time_ms);
#else
        do_DL_sig(r_re0,r_im0,r_re,r_im,s_re,s_im,eNB2UE,enb_data,ue_data,next_slot,abstraction_flag,frame_parms,UE_id);
#endif
      }
    }

    if ((direction  == SF_UL)|| (frame_parms->frame_type==0)) { //if ((subframe<2) || (subframe>4))
      do_UL_sig(r_re0,r_im0,r_re,r_im,s_re,s_im,UE2eNB,enb_data,ue_data,next_slot,abstraction_flag,frame_parms,frame);

      /*
            int ccc;
            fprintf(SINRpost,"SINRdb For eNB New Subframe : \n ");
            for(ccc = 0 ; ccc<301; ccc++)
            {
              fprintf(SINRpost,"_ %f ", SINRpost_eff[ccc]);
            }
            fprintf(SINRpost,"SINRdb For eNB : %f \n ", SINRpost_eff[ccc]);
            */
    }

    if ((direction == SF_S)) {//it must be a special subframe
      if (next_slot%2==0) {//DL part
        for (UE_id=0; UE_id<NB_UE_INST; UE_id++) {
#ifdef PAD
          pad_inject_job(0, UE_id, frame, next_slot, last_slot, JT_DL, oai_emulation.info.time_ms);
#else
          do_DL_sig(r_re0,r_im0,r_re,r_im,s_re,s_im,eNB2UE,enb_data,ue_data,next_slot,abstraction_flag,frame_parms,UE_id);
#endif
        }

        /*
              for (aarx=0;aarx<UE2eNB[1][0]->nb_rx;aarx++)
              for (aatx=0;aatx<UE2eNB[1][0]->nb_tx;aatx++)
              for (k=0;k<UE2eNB[1][0]->channel_length;k++)
              printf("SB(%d,%d,%d)->(%f,%f)\n",k,aarx,aatx,UE2eNB[1][0]->ch[aarx+(aatx*UE2eNB[1][0]->nb_rx)][k].r,UE2eNB[1][0]->ch[aarx+(aatx*UE2eNB[1][0]->nb_rx)][k].i);
            */
      } else { // UL part
        /*#ifdef PAD
        pthread_mutex_lock(&(pool->sync_lock));
        while(pool->active != 0) {
        pthread_cond_wait(&(pool->sync_notify), &(pool->sync_lock));
        }
        pthread_mutex_unlock(&(pool->sync_lock));
        #endif*/
        do_UL_sig(r_re0,r_im0,r_re,r_im,s_re,s_im,UE2eNB,enb_data,ue_data,next_slot,abstraction_flag,frame_parms,frame);
        /*
                int ccc;
                fprintf(SINRpost,"SINRdb For eNB New Subframe : \n ");
                for(ccc = 0 ; ccc<301; ccc++)
                {
                  fprintf(SINRpost,"_ %f ", SINRpost_eff[ccc]);
                }
                fprintf(SINRpost,"SINRdb For eNB : %f \n ", SINRpost_eff[ccc]);
                */
      }
    }

    if ((last_slot == 1) && (frame == 0)
        && (abstraction_flag == 0) && (oai_emulation.info.n_frames == 1)) {

      write_output ("dlchan0.m", "dlch0",
                    &(PHY_vars_UE_g[0]->lte_ue_common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[0][0][0]),
                    (6 * (PHY_vars_UE_g[0]->lte_frame_parms.ofdm_symbol_size)), 1, 1);
      write_output ("dlchan1.m", "dlch1",
                    &(PHY_vars_UE_g[0]->lte_ue_common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[1][0][0]),
                    (6 * (PHY_vars_UE_g[0]->lte_frame_parms.ofdm_symbol_size)), 1, 1);
      write_output ("dlchan2.m", "dlch2",
                    &(PHY_vars_UE_g[0]->lte_ue_common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[2][0][0]),
                    (6 * (PHY_vars_UE_g[0]->lte_frame_parms.ofdm_symbol_size)), 1, 1);
      write_output ("pbch_rxF_comp0.m", "pbch_comp0",
                    PHY_vars_UE_g[0]->lte_ue_pbch_vars[0]->rxdataF_comp[0], 6 * 12 * 4, 1, 1);
      write_output ("pbch_rxF_llr.m", "pbch_llr",
                    PHY_vars_UE_g[0]->lte_ue_pbch_vars[0]->llr, (frame_parms->Ncp == 0) ? 1920 : 1728, 1, 4);
    }

    if (next_slot %2 == 0) {
      clock_gettime (CLOCK_REALTIME, &time_spec);
      time_last = time_now;
      time_now = (unsigned long) time_spec.tv_nsec;
      td = (int) (time_now - time_last);

      if (td>0) {
        td_avg = (int)(((K*(long)td) + (((1<<3)-K)*((long)td_avg)))>>3); // in us
        LOG_T(EMU,"sleep frame %d, time_now %ldus,time_last %ldus,average time difference %ldns, CURRENT TIME DIFF %dus, avgerage difference from the target %dus\n",
              frame, time_now,time_last,td_avg, td/1000,(td_avg-TARGET_SF_TIME_NS)/1000);
      }

      if (td_avg<(TARGET_SF_TIME_NS - SF_DEVIATION_OFFSET_NS)) {
        sleep_time_us += SLEEP_STEP_US;
        LOG_D(EMU,"Faster than realtime increase the avg sleep time for %d us, frame %d\n",
              sleep_time_us,frame);
        // LOG_D(EMU,"Faster than realtime increase the avg sleep time for %d us, frame %d, time_now %ldus,time_last %ldus,average time difference %ldns, CURRENT TIME DIFF %dus, avgerage difference from the target %dus\n",    sleep_time_us,frame, time_now,time_last,td_avg, td/1000,(td_avg-TARGET_SF_TIME_NS)/1000);
      } else if (td_avg > (TARGET_SF_TIME_NS + SF_DEVIATION_OFFSET_NS)) {
        sleep_time_us-= SLEEP_STEP_US;
        LOG_D(EMU,"Slower than realtime reduce the avg sleep time for %d us, frame %d, time_now\n",
              sleep_time_us,frame);
        //LOG_T(EMU,"Slower than realtime reduce the avg sleep time for %d us, frame %d, time_now %ldus,time_last %ldus,average time difference %ldns, CURRENT TIME DIFF %dus, avgerage difference from the target %dus\n",     sleep_time_us,frame, time_now,time_last,td_avg, td/1000,(td_avg-TARGET_SF_TIME_NS)/1000);
      }
    } // end if next_slot%2

    slot++;

    if (slot == 20) { //Frame's Epilogue
      frame++;
      slot = 0;

      // if n_frames not set by the user or is greater than max num frame then set adjust the frame counter
      if ( (oai_emulation.info.n_frames_flag == 0) || (oai_emulation.info.n_frames >= 0xffff) ) {
        frame %=(oai_emulation.info.n_frames-1);
      }

      oai_emulation.info.time_s += 0.01;

      if ((frame>=1)&&(frame<=9)&&(abstraction_flag==0)) {
        write_output("UEtxsig0.m","txs0", PHY_vars_UE_g[0]->lte_ue_common_vars.txdata[0],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);
        sprintf(fname,"eNBtxsig%d.m",frame);
        sprintf(vname,"txs%d",frame);
        write_output(fname,vname, PHY_vars_eNB_g[0]->lte_eNB_common_vars.txdata[0][0],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);
        write_output("eNBtxsigF0.m","txsF0",PHY_vars_eNB_g[0]->lte_eNB_common_vars.txdataF[0][0],PHY_vars_eNB_g[0]->lte_frame_parms.symbols_per_tti*PHY_vars_eNB_g[0]->lte_frame_parms.ofdm_symbol_size,1,1);

        write_output("UErxsig0.m","rxs0", PHY_vars_UE_g[0]->lte_ue_common_vars.rxdata[0],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);
        write_output("eNBrxsig0.m","rxs0", PHY_vars_eNB_g[0]->lte_eNB_common_vars.rxdata[0][0],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);
      }

#ifdef XFORMS
      do_xforms();
#endif

      // calibrate at the end of each frame if there is some time  left
      if((sleep_time_us > 0)&& (ethernet_flag ==0)) {
        LOG_I(EMU,"[TIMING] Adjust average frame duration, sleep for %d us\n",sleep_time_us);
        usleep(sleep_time_us);
        sleep_time_us=0; // reset the timer, could be done per n SF
      }

#ifdef SMBV

      if ((frame == config_frames[0]) || (frame == config_frames[1]) || (frame == config_frames[2]) || (frame == config_frames[3])) {
        smbv_frame_cnt++;
      }

#endif
    }
  }

  t = clock() - t;
  printf("rrc Duration of the simulation: %f seconds\n",((float)t)/CLOCKS_PER_SEC);

  fclose(SINRpost);
  LOG_I(EMU,">>>>>>>>>>>>>>>>>>>>>>>>>>> OAIEMU Ending <<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

  free(otg_pdcp_buffer);

#ifdef SMBV

  if (config_smbv) {
    smbv_send_config (smbv_fname,smbv_ip);
  }

#endif

  //Perform KPI measurements
  if (oai_emulation.info.otg_enabled==1)
    kpi_gen();

#ifdef PAD
  pad_finalize();
#endif

  // relase all rx state
  if (ethernet_flag == 1) {
    emu_transport_release ();
  }

  if (abstraction_flag == 0) {
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

    for (i = 0; i < 2; i++) {
      free (s_re[i]);
      free (s_im[i]);
      free (r_re[i]);
      free (r_im[i]);
    }

    free (s_re);
    free (s_im);
    free (r_re);
    free (r_im);
    lte_sync_time_free ();
  }

  //  pthread_join(sigth, NULL);

  // added for PHY abstraction
  if (oai_emulation.info.ocm_enabled == 1) {
    for (eNB_id = 0; eNB_id < NUMBER_OF_eNB_MAX; eNB_id++)
      free(enb_data[eNB_id]);

    for (UE_id = 0; UE_id < NUMBER_OF_UE_MAX; UE_id++)
      free(ue_data[UE_id]);
  } //End of PHY abstraction changes

#ifdef OPENAIR2
  mac_top_cleanup();
#endif

#ifdef PRINT_STATS

  for(UE_id=0; UE_id<NB_UE_INST; UE_id++) {
    if (UE_stats[UE_id])
      fclose (UE_stats[UE_id]);

    if(UE_stats_th[UE_id])
      fclose (UE_stats_th[UE_id]);
  }

  if (eNB_stats)
    fclose (eNB_stats);

  if (eNB_avg_thr)
    fclose (eNB_avg_thr);

  if (eNB_l2_stats)
    fclose (eNB_l2_stats);

#endif

  // stop OMG
  stop_mobility_generator(oai_emulation.info.omg_model_ue);//omg_param_list.mobility_type
#ifdef OPENAIR2

  if (oai_emulation.info.omv_enabled == 1)
    omv_end(pfd[1],omv_data);

#endif

  if ((oai_emulation.info.ocm_enabled == 1) && (ethernet_flag == 0) && (ShaF != NULL))
    destroyMat(ShaF,map1, map2);

  if (opt_enabled == 1 )
    terminate_opt();

  if (oai_emulation.info.cli_enabled)
    cli_server_cleanup();

  //bring oai if down
  terminate();
  logClean();
  VCD_SIGNAL_DUMPER_CLOSE();
  //printf("FOR MAIN TIMES = %d &&&& OTG TIMES = %d <-> FOR TIMES = %d <-> IF TIMES = %d\n", for_main_times, otg_times, for_times, if_times);

}
//<<PAD>>//

void terminate(void)
{
  int i;
  char interfaceName[8];

  for (i=0; i < NUMBER_OF_eNB_MAX+NUMBER_OF_UE_MAX; i++)
    if (oai_emulation.info.oai_ifup[i]==1) {
      sprintf(interfaceName, "oai%d", i);
      bringInterfaceUp(interfaceName,0);
    }
}

#ifdef OPENAIR2
int omv_write (int pfd,  Node_list enb_node_list, Node_list ue_node_list, Data_Flow_Unit omv_data)
{
  int i,j;
  omv_data.end=0;

  //omv_data.total_num_nodes = NB_UE_INST + NB_eNB_INST;
  for (i=0; i<NB_eNB_INST; i++) {
    if (enb_node_list != NULL) {
      omv_data.geo[i].x = (enb_node_list->node->X_pos < 0.0)? 0.0 : enb_node_list->node->X_pos;
      omv_data.geo[i].y = (enb_node_list->node->Y_pos < 0.0)? 0.0 : enb_node_list->node->Y_pos;
      omv_data.geo[i].z = 1.0;
      omv_data.geo[i].mobility_type = oai_emulation.info.omg_model_enb;
      omv_data.geo[i].node_type = 0; //eNB
      enb_node_list = enb_node_list->next;
      omv_data.geo[i].Neighbors=0;

      for (j=NB_eNB_INST; j< NB_UE_INST + NB_eNB_INST ; j++) {
        if (is_UE_active(i,j - NB_eNB_INST ) == 1) {
          omv_data.geo[i].Neighbor[omv_data.geo[i].Neighbors]=  j;
          omv_data.geo[i].Neighbors++;
          LOG_D(OMG,"[eNB %d][UE %d] is_UE_active(i,j) %d geo (x%d, y%d) num neighbors %d\n", i,j-NB_eNB_INST, is_UE_active(i,j-NB_eNB_INST),
                omv_data.geo[i].x, omv_data.geo[i].y, omv_data.geo[i].Neighbors);
        }
      }
    }
  }

  for (i=NB_eNB_INST; i<NB_UE_INST+NB_eNB_INST; i++) {
    if (ue_node_list != NULL) {
      omv_data.geo[i].x = (ue_node_list->node->X_pos < 0.0) ? 0.0 : ue_node_list->node->X_pos;
      omv_data.geo[i].y = (ue_node_list->node->Y_pos < 0.0) ? 0.0 : ue_node_list->node->Y_pos;
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
      omv_data.geo[i].RSSI[2] = 11;

      ue_node_list = ue_node_list->next;
      omv_data.geo[i].Neighbors=0;

      for (j=0; j< NB_eNB_INST ; j++) {
        if (is_UE_active(j,i-NB_eNB_INST) == 1) {
          omv_data.geo[i].Neighbor[ omv_data.geo[i].Neighbors]=j;
          omv_data.geo[i].Neighbors++;
          LOG_D(OMG,"[UE %d][eNB %d] is_UE_active  %d geo (x%d, y%d) num neighbors %d\n", i-NB_eNB_INST,j, is_UE_active(j,i-NB_eNB_INST),
                omv_data.geo[i].x, omv_data.geo[i].y, omv_data.geo[i].Neighbors);
        }
      }
    }
  }

  if( write( pfd, &omv_data, sizeof(struct Data_Flow_Unit) ) == -1 )
    perror( "write omv failed" );

  return 1;
}

void omv_end (int pfd, Data_Flow_Unit omv_data)
{
  omv_data.end=1;

  if( write( pfd, &omv_data, sizeof(struct Data_Flow_Unit) ) == -1 )
    perror( "write omv failed" );
}
#endif
