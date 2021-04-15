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

/*! \file dlsim.c
 \brief Top-level DL simulator
 \author R. Knopp
 \date 2011 - 2014
 \version 0.1
 \company Eurecom
 \email: knopp@eurecom.fr
 \note
 \warning
*/

#include <string.h>
#include <math.h>
#include <unistd.h>
#include <execinfo.h>
#include <signal.h>

#include "SIMULATION/TOOLS/defs.h"
#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/vars.h"

#include "SCHED/defs.h"
#include "SCHED/vars.h"
#include "LAYER2/MAC/vars.h"

#include "OCG_vars.h"
#include "UTIL/LOG/log.h"
#include "UTIL/LISTS/list.h"

#include "unitary_defs.h"

extern unsigned int dlsch_tbs25[27][25],TBStable[27][110];
extern unsigned char offset_mumimo_llr_drange_fix;

#include "PHY/TOOLS/lte_phy_scope.h"

PHY_VARS_eNB *eNB;
PHY_VARS_UE *UE;

int otg_enabled=0;
/*the following parameters are used to control the processing times calculations*/
double t_tx_max = -1000000000; /*!< \brief initial max process time for tx */
double t_rx_max = -1000000000; /*!< \brief initial max process time for rx */
double t_tx_min = 1000000000; /*!< \brief initial min process time for tx */
double t_rx_min = 1000000000; /*!< \brief initial min process time for rx */
int n_tx_dropped = 0; /*!< \brief initial max process time for tx */
int n_rx_dropped = 0; /*!< \brief initial max process time for rx */

void handler(int sig) {
  void *array[10];
  size_t size;
  // get void*'s for all entries on the stack
  size = backtrace(array, 10);
  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, 2);
  exit(1);
}



//DCI2_5MHz_2A_M10PRB_TDD_t DLSCH_alloc_pdu2_2A[2];

DCI1E_5MHz_2A_M10PRB_TDD_t  DLSCH_alloc_pdu2_1E[2];
uint64_t DLSCH_alloc_pdu_1[2];

#define UL_RB_ALLOC 0x1ff;
#define CCCH_RB_ALLOC computeRIV(eNB->frame_parms.N_RB_UL,0,2)
//#define DLSCH_RB_ALLOC 0x1fbf // igore DC component,RB13
//#define DLSCH_RB_ALLOC 0x0001

int main(int argc, char **argv) {
  int c;
  int k,i,j,aa,aarx,aatx;
  int re;
  int s,Kr,Kr_bytes;
  double sigma2, sigma2_dB=10,SNR,snr0=-2.0,snr1,rate;
  double snr_step=1,input_snr_step=1, snr_int=30;
  LTE_DL_FRAME_PARMS *frame_parms;
  double **s_re,**s_im,**r_re,**r_im;
  double forgetting_factor=0.0; //in [0,1] 0 means a new channel every time, 1 means keep the same channel
  double iqim=0.0;
  uint8_t extended_prefix_flag=0,transmission_mode=1,n_tx_port=1,n_tx_phy=1,n_rx=1;
  uint16_t Nid_cell=0;
  int32_t **cell_spec_bf_weights;
  int32_t *ue_spec_bf_weights;
  int eNB_id = 0, eNB_id_i = 1;
  unsigned char mcs1=0,mcs2=0,mcs_i=0,dual_stream_UE = 0,awgn_flag=0,round,dci_flag=0;
  unsigned char i_mod = 2;
  unsigned short NB_RB;
  unsigned char Ns,l,m;
  uint16_t tdd_config=3;
  uint16_t n_rnti=0x1234;
  int n_users = 1;
  SCM_t channel_model=Rayleigh1;
  //  unsigned char *input_data,*decoded_output;
  unsigned char *input_buffer0[2],*input_buffer1[2];
  unsigned short input_buffer_length0,input_buffer_length1;
  unsigned int ret;
  unsigned int coded_bits_per_codeword=0,nsymb,dci_cnt,tbs=0;
  unsigned int tx_lev=0,tx_lev_dB=0,trials,errs[4]= {0,0,0,0},errs2[4]= {0,0,0,0},round_trials[4]= {0,0,0,0},dci_errors=0,dlsch_active=0;//,num_layers;
  int re_allocated;
  char fname[32],vname[32];
  FILE *bler_fd;
  char bler_fname[256];
  FILE *time_meas_fd;
  char time_meas_fname[256];
  //  FILE *tikz_fd;
  //  char tikz_fname[256];
  FILE *input_trch_fd=NULL;
  unsigned char input_trch_file=0;
  FILE *input_fd=NULL;
  unsigned char input_file=0;
  //  char input_val_str[50],input_val_str2[50];
  char input_trch_val[16];
  double channelx,channely;
  unsigned char pbch_pdu[6];
  DCI_ALLOC_t dci_alloc[8],dci_alloc_rx[8];
  int num_common_dci=0,num_ue_spec_dci=0,num_dci=0;
  //  FILE *rx_frame_file;
  int n_frames;
  int n_ch_rlz = 1;
  channel_desc_t *eNB2UE[4];
  uint8_t num_pdcch_symbols=1,num_pdcch_symbols_2=0;
  uint8_t pilot1,pilot2,pilot3;
  uint8_t rx_sample_offset = 0;
  //char stats_buffer[4096];
  //int len;
  uint8_t num_rounds = 4;//,fix_rounds=0;
  uint8_t subframe=7;
  int u;
  int n=0;
  int abstx=0;
  int iii;
  FILE *csv_fd=NULL;
  char csv_fname[512];
  int ch_realization;
  int pmi_feedback=0;
  int hold_channel=0;
  // void *data;
  // int ii;
  //  int bler;
  double blerr[4],uncoded_ber,avg_ber;
  short *uncoded_ber_bit=NULL;
  uint8_t N_RB_DL=25,osf=1;
  frame_t frame_type = FDD;
  int xforms=0;
  FD_lte_phy_scope_ue *form_ue = NULL;
  char title[255];
  uint32_t DLSCH_RB_ALLOC = 0x1fff;
  int numCCE=0;
  int dci_length_bytes=0,dci_length=0;
  //double channel_bandwidth = 5.0, sampling_rate=7.68;
  int common_flag=0,TPC=0;
  double cpu_freq_GHz;
  //  time_stats_t ts;//,sts,usts;
  int avg_iter,iter_trials;
  int rballocset=0;
  int print_perf=0;
  int test_perf=0;
  int dump_table=0;
  int llr8_flag=0;
  double effective_rate=0.0;
  char channel_model_input[10]="I";
  int TB0_active = 1;
  uint32_t perfect_ce = 0;
  //  LTE_DL_UE_HARQ_t *dlsch0_ue_harq;
  //  LTE_DL_eNB_HARQ_t *dlsch0_eNB_harq;
  uint8_t Kmimo;
  uint8_t ue_category=4;
  uint32_t Nsoft;
  int CCE_table[800];
  int threequarter_fs=0;
  opp_enabled=1; // to enable the time meas
#if defined(__arm__)
  FILE    *proc_fd = NULL;
  char buf[64];
  proc_fd = fopen("/sys/devices/system/cpu/cpu4/cpufreq/cpuinfo_cur_freq", "r");

  if(!proc_fd) {
    printf("cannot open /sys/devices/system/cpu/cpu4/cpufreq/cpuinfo_cur_freq");
    exit(-1);
  } else {
    while(fgets(buf, 63, proc_fd))
      printf("%s", buf);
  }

  fclose(proc_fd);
  cpu_freq_GHz = ((double)atof(buf))/1e6;
#else
  cpu_freq_GHz = get_cpu_freq_GHz();
#endif
  printf("Detected cpu_freq %f GHz\n",cpu_freq_GHz);
  //signal(SIGSEGV, handler);
  //signal(SIGABRT, handler);
  logInit();
  // default parameters
  n_frames = 1000;
  snr0 = 0;
  //  num_layers = 1;
  perfect_ce = 0;

  while ((c = getopt (argc, argv, "ahdpZDe:Em:n:o:s:f:t:c:g:r:F:x:p:y:z:AM:N:I:i:O:R:S:C:T:b:u:v:w:B:PLl:XY")) != -1) {
    switch (c) {
      case 'a':
        awgn_flag = 1;
        channel_model = AWGN;
        break;

      case 'A':
        abstx = 1;
        break;

      case 'b':
        tdd_config=atoi(optarg);
        break;

      case 'B':
        N_RB_DL=atoi(optarg);
        break;

      case 'c':
        num_pdcch_symbols=atoi(optarg);
        break;

      case 'C':
        Nid_cell = atoi(optarg);
        break;

      case 'd':
        dci_flag = 1;
        break;

      case 'D':
        frame_type=TDD;
        break;

      case 'e':
        num_rounds=1;
        common_flag = 1;
        TPC = atoi(optarg);
        break;

      case 'E':
        threequarter_fs=1;
        break;

      case 'f':
        input_snr_step= atof(optarg);
        break;

      case 'F':
        forgetting_factor = atof(optarg);
        break;

      case 'i':
        input_fd = fopen(optarg,"r");
        input_file=1;
        dci_flag = 1;
        break;

      case 'I':
        input_trch_fd = fopen(optarg,"r");
        input_trch_file=1;
        break;

      case 'L':
        llr8_flag=1;
        break;

      case 'l':
        offset_mumimo_llr_drange_fix=atoi(optarg);
        break;

      case 'm':
        mcs1 = atoi(optarg);
        break;

      case 'M':
        mcs2 = atoi(optarg);
        break;

      case 'O':
        test_perf=atoi(optarg);
        //print_perf =1;
        break;

      case 't':
        mcs_i = atoi(optarg);
        i_mod = get_Qm(mcs_i);
        break;

      case 'n':
        n_frames = atoi(optarg);
        break;

      case 'o':
        rx_sample_offset = atoi(optarg);
        break;

      case 'r':
        DLSCH_RB_ALLOC = atoi(optarg);
        rballocset = 1;
        break;

      case 's':
        snr0 = atof(optarg);
        break;

      case 'w':
        snr_int = atof(optarg);
        break;

      case 'N':
        n_ch_rlz= atof(optarg);
        break;

      case 'p':
        extended_prefix_flag=1;
        break;

      case 'g':
        memcpy(channel_model_input,optarg,10);

        switch((char)*optarg) {
          case 'A':
            channel_model=SCM_A;
            break;

          case 'B':
            channel_model=SCM_B;
            break;

          case 'C':
            channel_model=SCM_C;
            break;

          case 'D':
            channel_model=SCM_D;
            break;

          case 'E':
            channel_model=EPA;
            break;

          case 'F':
            channel_model=EVA;
            break;

          case 'G':
            channel_model=ETU;
            break;

          case 'H':
            channel_model=Rayleigh8;
            break;

          case 'I':
            channel_model=Rayleigh1;
            break;

          case 'J':
            channel_model=Rayleigh1_corr;
            break;

          case 'K':
            channel_model=Rayleigh1_anticorr;
            break;

          case 'L':
            channel_model=Rice8;
            break;

          case 'M':
            channel_model=Rice1;
            break;

          case 'N':
            channel_model=AWGN;
            break;

          default:
            msg("Unsupported channel model!\n");
            exit(-1);
        }

        break;

      case 'R':
        num_rounds=atoi(optarg);
        break;

      case 'S':
        subframe=atoi(optarg);
        break;

      case 'T':
        n_rnti=atoi(optarg);
        break;

      case 'u':
        dual_stream_UE=1;
        UE->use_ia_receiver = 1;

        if ((n_tx_port!=2) || (transmission_mode!=5)) {
          msg("IA receiver only supported for TM5!");
          exit(-1);
        }

        break;

      case 'v':
        i_mod = atoi(optarg);

        if (i_mod!=2 && i_mod!=4 && i_mod!=6) {
          msg("Wrong i_mod %d, should be 2,4 or 6\n",i_mod);
          exit(-1);
        }

        break;

      case 'P':
        print_perf=1;
        break;

      case 'q':
        n_tx_port=atoi(optarg);

        if ((n_tx_port==0) || ((n_tx_port>2))) {
          msg("Unsupported number of cell specific antennas ports %d\n",n_tx_port);
          exit(-1);
        }

        break;

      case 'x':
        transmission_mode=atoi(optarg);

        if ((transmission_mode!=1) &&
            (transmission_mode!=2) &&
            (transmission_mode!=3) &&
            (transmission_mode!=4) &&
            (transmission_mode!=5) &&
            (transmission_mode!=6) &&
            (transmission_mode!=7)) {
          msg("Unsupported transmission mode %d\n",transmission_mode);
          exit(-1);
        }

        if (transmission_mode>1 && transmission_mode<7) {
          n_tx_port = 2;
        }

        break;

      case 'y':
        n_tx_phy=atoi(optarg);

        if (transmission_mode>1&&transmission_mode<7) {
          if(n_tx_phy==1) {
            msg("n_tx_phy must be >1 for transmission_mode %d\n",transmission_mode);
            exit(-1);
          }
        }

        if (transmission_mode==7 && (n_tx_phy!=1 && n_tx_phy!=2 && n_tx_phy!=4 && n_tx_phy!=8 && n_tx_phy!=16 && n_tx_phy!=64 && n_tx_phy!=128)) {
          msg("Physical number of antennas not supported for TM7.\n");
          exit(-1);
        }

        break;

      case 'X':
        xforms=1;
        break;

      case 'Y':
        perfect_ce=1;
        break;

      case 'z':
        n_rx=atoi(optarg);

        if ((n_rx==0) || (n_rx>2)) {
          msg("Unsupported number of rx antennas %d\n",n_rx);
          exit(-1);
        }

        break;

      case 'Z':
        dump_table=1;
        break;

      case 'h':
      default:
        printf("%s -h(elp) -a(wgn on) -d(ci decoding on) -p(extended prefix on) -m mcs1 -M mcs2 -n n_frames -s snr0 -x transmission mode (1,2,5,6,7) -y TXant -z RXant -I trch_file\n",argv[0]);
        printf("-h This message\n");
        printf("-a Use AWGN channel and not multipath\n");
        printf("-c Number of PDCCH symbols\n");
        printf("-m MCS1 for TB 1\n");
        printf("-M MCS2 for TB 2\n");
        printf("-d Transmit the DCI and compute its error statistics and the overall throughput\n");
        printf("-p Use extended prefix mode\n");
        printf("-n Number of frames to simulate\n");
        printf("-o Sample offset for receiver\n");
        printf("-s Starting SNR, runs from SNR to SNR+%.1fdB in steps of %.1fdB. If n_frames is 1 then just SNR is simulated and MATLAB/OCTAVE output is generated\n", snr_int, snr_step);
        printf("-f step size of SNR, default value is 1.\n");
        printf("-r ressource block allocation (see  section 7.1.6.3 in 36.213\n");
        printf("-g [A:M] Use 3GPP 25.814 SCM-A/B/C/D('A','B','C','D') or 36-101 EPA('E'), EVA ('F'),ETU('G') models (ignores delay spread and Ricean factor), Rayghleigh8 ('H'), Rayleigh1('I'), Rayleigh1_corr('J'), Rayleigh1_anticorr ('K'), Rice8('L'), Rice1('M')\n");
        printf("-F forgetting factor (0 new channel every trial, 1 channel constant\n");
        printf("-x Transmission mode (1,2,6,7 for the moment)\n");
        printf("-q Number of TX antennas ports used in eNB\n");
        printf("-y Number of physical TX antennas used in eNB\n");
        printf("-z Number of RX antennas used in UE\n");
        printf("-t MCS of interfering UE\n");
        printf("-R Number of HARQ rounds (fixed)\n");
        printf("-A Turns on calibration mode for abstraction.\n");
        printf("-N Determines the number of Channel Realizations in Abstraction mode. Default value is 1. \n");
        printf("-O Set the percenatge of effective rate to testbench the modem performance (typically 30 and 70, range 1-100) \n");
        printf("-I Input filename for TrCH data (binary)\n");
        printf("-u Enables the Interference Aware Receiver for TM5 (default is normal receiver)\n");
        exit(1);
        break;
    }
  }

  if (common_flag == 0) {
    switch (N_RB_DL) {
      case 6:
        if (rballocset==0) DLSCH_RB_ALLOC = 0x3f;

        num_pdcch_symbols = 3;
        break;

      case 25:
        if (rballocset==0) DLSCH_RB_ALLOC = 0x1fff;

        break;

      case 50:
        if (rballocset==0) DLSCH_RB_ALLOC = 0x1ffff;

        break;

      case 100:
        if (rballocset==0) DLSCH_RB_ALLOC = 0x1ffffff;

        break;
    }

    NB_RB=conv_nprb(0,DLSCH_RB_ALLOC,N_RB_DL);
  } else
    NB_RB = 4;

  if (xforms==1) {
    fl_initialize (&argc, argv, NULL, 0, 0);
    form_ue = create_lte_phy_scope_ue();
    sprintf (title, "LTE PHY SCOPE eNB");
    fl_show_form (form_ue->lte_phy_scope_ue, FL_PLACE_HOTSPOT, FL_FULLBORDER, title);

    if (!dual_stream_UE==0) {
      UE->use_ia_receiver = 1;
      fl_set_button(form_ue->button_0,1);
      fl_set_object_label(form_ue->button_0, "IA Receiver ON");
      fl_set_object_color(form_ue->button_0, FL_GREEN, FL_GREEN);
    }
  }

  if (transmission_mode==5) {
    n_users = 2;
    printf("dual_stream_UE=%d\n", dual_stream_UE);
  }

  lte_param_init(n_tx_port,n_tx_phy,n_rx,transmission_mode,extended_prefix_flag,frame_type,Nid_cell,tdd_config,N_RB_DL,threequarter_fs,osf,perfect_ce);
  frame_parms = &eNB->frame_parms;
  /*
  cell_spec_bf_weights = eNB->common_vars.beam_weights[0][0];
  for(aa=0;aa<n_tx_phy;aa++) {
    for(re=0;re<frame_parms->ofdm_symbol_size;re++) {
      if (n_tx_phy==1 || n_tx_phy==2)
        cell_spec_bf_weights[aa][re] = 0x00007fff;
      else if (n_tx_phy==4 || n_tx_phy==8)
        cell_spec_bf_weights[aa][re] = 0x00007fff>>1;
      else if (n_tx_phy==16)
        cell_spec_bf_weights[aa][re] = 0x00007fff>>2;
      else if (n_tx_phy==64)
        cell_spec_bf_weights[aa][re] = 0x00007fff>>4;
    }
  }

  if (transmission_mode==7){
    lte_gold_ue_spec_port5(eNB->lte_gold_uespec_port5_table[0],Nid_cell,n_rnti);
    lte_gold_ue_spec_port5(UE->lte_gold_uespec_port5_table,Nid_cell,n_rnti);
  }*/
  eNB_id_i = UE->n_connected_eNB;
  printf("Setting mcs1 = %d\n",mcs1);
  printf("Setting mcs2 = %d\n",mcs2);
  printf("NPRB = %d\n",NB_RB);
  printf("n_frames = %d\n",n_frames);
  printf("Transmission mode %d with %dx%d antenna configuration, Extended Prefix %d\n",transmission_mode,n_tx_phy,n_rx,extended_prefix_flag);
  snr1 = snr0+snr_int;
  printf("SNR0 %f, SNR1 %f\n",snr0,snr1);
  s_re = malloc(n_tx_phy*sizeof(double *));
  s_im = malloc(n_tx_phy*sizeof(double *));
  r_re = malloc(2*sizeof(double *));
  r_im = malloc(2*sizeof(double *));
  //  r_re0 = malloc(2*sizeof(double*));
  //  r_im0 = malloc(2*sizeof(double*));
  nsymb = (eNB->frame_parms.Ncp == 0) ? 14 : 12;
  printf("Channel Model= (%s,%d)\n",channel_model_input, channel_model);
  printf("SCM-A=%d, SCM-B=%d, SCM-C=%d, SCM-D=%d, EPA=%d, EVA=%d, ETU=%d, Rayleigh8=%d, Rayleigh1=%d, Rayleigh1_corr=%d, Rayleigh1_anticorr=%d, Rice1=%d, Rice8=%d\n",
         SCM_A, SCM_B, SCM_C, SCM_D, EPA, EVA, ETU, Rayleigh8, Rayleigh1, Rayleigh1_corr, Rayleigh1_anticorr, Rice1, Rice8);

  if(transmission_mode==5)
    sprintf(bler_fname,"bler_tm%d_chan%d_perfce%d_ntx%d_nrx%d_mcs%d_mcsi%d_u%d_imod%d.csv",transmission_mode,channel_model,perfect_ce,n_tx_phy,n_rx,mcs1,mcs_i,dual_stream_UE,i_mod);
  else
    sprintf(bler_fname,"bler_tm%d_chan%d_perfce%d_ntx%d_nrx%d_mcs%d.csv",transmission_mode,channel_model,perfect_ce,n_tx_phy,n_rx,mcs1);

  bler_fd = fopen(bler_fname,"w");

  if (bler_fd==NULL) {
    fprintf(stderr,"Cannot create file %s!\n",bler_fname);
    exit(-1);
  }

  fprintf(bler_fd,"SNR; MCS; TBS; rate; err0; trials0; err1; trials1; err2; trials2; err3; trials3; dci_err\n");

  if (test_perf != 0) {
    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);
    printf("Hostname: %s\n", hostname);
    //char dirname[FILENAME_MAX];
    //sprintf(dirname, "%s/SIMU/USER/pre-ci-logs-%s", getenv("OPENAIR_TARGETS"),hostname );
    sprintf(time_meas_fname,"time_meas_prb%d_mcs%d_anttx%d_antrx%d_pdcch%d_channel%s_tx%d.csv",
            N_RB_DL,mcs1,n_tx_phy,n_rx,num_pdcch_symbols,channel_model_input,transmission_mode);
    //mkdir(dirname,0777);
    time_meas_fd = fopen(time_meas_fname,"w");

    if (time_meas_fd==NULL) {
      fprintf(stderr,"Cannot create file %s!\n",time_meas_fname);
      exit(-1);
    }
  }

  if(abstx) {
    // CSV file
    sprintf(csv_fname,"dataout_tx%d_u2%d_mcs%d_chan%d_nsimus%d_R%d.m",transmission_mode,dual_stream_UE,mcs1,channel_model,n_frames,num_rounds);
    csv_fd = fopen(csv_fname,"w");
    if (csv_fd==NULL) {
      fprintf(stderr,"Cannot create file %s!\n",csv_fname);
      exit(-1);
    }
    fprintf(csv_fd,"data_all%d=[",mcs1);
  }

  /*
  //sprintf(tikz_fname, "second_bler_tx%d_u2=%d_mcs%d_chan%d_nsimus%d.tex",transmission_mode,dual_stream_UE,mcs,channel_model,n_frames);
  sprintf(tikz_fname, "second_bler_tx%d_u2%d_mcs%d_chan%d_nsimus%d",transmission_mode,dual_stream_UE,mcs,channel_model,n_frames);
  tikz_fd = fopen(tikz_fname,"w");
  //fprintf(tikz_fd,"\\addplot[color=red, mark=o] plot coordinates {");
  switch (mcs)
    {
    case 0:
      fprintf(tikz_fd,"\\addplot[color=blue, mark=star] plot coordinates {");
      break;
    case 1:
      fprintf(tikz_fd,"\\addplot[color=red, mark=star] plot coordinates {");
      break;
    case 2:
      fprintf(tikz_fd,"\\addplot[color=green, mark=star] plot coordinates {");
      break;
    case 3:
      fprintf(tikz_fd,"\\addplot[color=yellow, mark=star] plot coordinates {");
      break;
    case 4:
      fprintf(tikz_fd,"\\addplot[color=black, mark=star] plot coordinates {");
      break;
    case 5:
      fprintf(tikz_fd,"\\addplot[color=blue, mark=o] plot coordinates {");
      break;
    case 6:
      fprintf(tikz_fd,"\\addplot[color=red, mark=o] plot coordinates {");
      break;
    case 7:
      fprintf(tikz_fd,"\\addplot[color=green, mark=o] plot coordinates {");
      break;
    case 8:
      fprintf(tikz_fd,"\\addplot[color=yellow, mark=o] plot coordinates {");
      break;
    case 9:
      fprintf(tikz_fd,"\\addplot[color=black, mark=o] plot coordinates {");
      break;
    case 10:
      fprintf(tikz_fd,"\\addplot[color=blue, mark=square] plot coordinates {");
      break;
    case 11:
      fprintf(tikz_fd,"\\addplot[color=red, mark=square] plot coordinates {");
      break;
    case 12:
      fprintf(tikz_fd,"\\addplot[color=green, mark=square] plot coordinates {");
      break;
    case 13:
      fprintf(tikz_fd,"\\addplot[color=yellow, mark=square] plot coordinates {");
      break;
    case 14:
      fprintf(tikz_fd,"\\addplot[color=black, mark=square] plot coordinates {");
      break;
    case 15:
      fprintf(tikz_fd,"\\addplot[color=blue, mark=diamond] plot coordinates {");
      break;
    case 16:
      fprintf(tikz_fd,"\\addplot[color=red, mark=diamond] plot coordinates {");
      break;
    case 17:
      fprintf(tikz_fd,"\\addplot[color=green, mark=diamond] plot coordinates {");
      break;
    case 18:
      fprintf(tikz_fd,"\\addplot[color=yellow, mark=diamond] plot coordinates {");
      break;
    case 19:
      fprintf(tikz_fd,"\\addplot[color=black, mark=diamond] plot coordinates {");
      break;
    case 20:
      fprintf(tikz_fd,"\\addplot[color=blue, mark=x] plot coordinates {");
      break;
    case 21:
      fprintf(tikz_fd,"\\addplot[color=red, mark=x] plot coordinates {");
      break;
    case 22:
      fprintf(tikz_fd,"\\addplot[color=green, mark=x] plot coordinates {");
      break;
    case 23:
      fprintf(tikz_fd,"\\addplot[color=yellow, mark=x] plot coordinates {");
      break;
    case 24:
      fprintf(tikz_fd,"\\addplot[color=black, mark=x] plot coordinates {");
      break;
    case 25:
      fprintf(tikz_fd,"\\addplot[color=blue, mark=x] plot coordinates {");
      break;
    case 26:
      fprintf(tikz_fd,"\\addplot[color=red, mark=+] plot coordinates {");
      break;
    case 27:
      fprintf(tikz_fd,"\\addplot[color=green, mark=+] plot coordinates {");
      break;
    case 28:
      fprintf(tikz_fd,"\\addplot[color=yellow, mark=+] plot coordinates {");
      break;
    }
  */

  for (i=0; i<n_tx_phy; i++) {
    s_re[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    s_im[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
  }

  for (i=0; i<2; i++) {
    r_re[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    r_im[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    //    r_re0[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    //    bzero(r_re0[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    //    r_im0[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    //    bzero(r_im0[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
  }

  UE->pdcch_vars[0]->crnti = n_rnti;
  // Fill in UL_alloc
  UL_alloc_pdu.type    = 0;
  UL_alloc_pdu.hopping = 0;
  UL_alloc_pdu.rballoc = UL_RB_ALLOC;
  UL_alloc_pdu.mcs     = 1;
  UL_alloc_pdu.ndi     = 1;
  UL_alloc_pdu.TPC     = 0;
  UL_alloc_pdu.cqi_req = 1;
  CCCH_alloc_pdu.type               = 0;
  CCCH_alloc_pdu.vrb_type           = 0;
  CCCH_alloc_pdu.rballoc            = CCCH_RB_ALLOC;
  CCCH_alloc_pdu.ndi      = 1;
  CCCH_alloc_pdu.mcs      = 1;
  CCCH_alloc_pdu.harq_pid = 0;
  DLSCH_alloc_pdu2_1E[0].rah              = 0;
  DLSCH_alloc_pdu2_1E[0].rballoc          = DLSCH_RB_ALLOC;
  DLSCH_alloc_pdu2_1E[0].TPC              = 0;
  DLSCH_alloc_pdu2_1E[0].dai              = 0;
  DLSCH_alloc_pdu2_1E[0].harq_pid         = 0;
  //DLSCH_alloc_pdu2_1E[0].tb_swap          = 0;
  DLSCH_alloc_pdu2_1E[0].mcs             = mcs1;
  DLSCH_alloc_pdu2_1E[0].ndi             = 1;
  DLSCH_alloc_pdu2_1E[0].rv              = 0;
  // Forget second codeword
  DLSCH_alloc_pdu2_1E[0].tpmi             = (transmission_mode>=5 ? 5 : 0);  // precoding
  DLSCH_alloc_pdu2_1E[0].dl_power_off     = (transmission_mode==5 ? 0 : 1);
  DLSCH_alloc_pdu2_1E[1].rah              = 0;
  DLSCH_alloc_pdu2_1E[1].rballoc          = DLSCH_RB_ALLOC;
  DLSCH_alloc_pdu2_1E[1].TPC              = 0;
  DLSCH_alloc_pdu2_1E[1].dai              = 0;
  DLSCH_alloc_pdu2_1E[1].harq_pid         = 0;
  //DLSCH_alloc_pdu2_1E[1].tb_swap          = 0;
  DLSCH_alloc_pdu2_1E[1].mcs             = mcs_i;
  DLSCH_alloc_pdu2_1E[1].ndi             = 1;
  DLSCH_alloc_pdu2_1E[1].rv              = 0;
  // Forget second codeword
  DLSCH_alloc_pdu2_1E[1].tpmi             = (transmission_mode>=5 ? 5 : 0) ;  // precoding
  DLSCH_alloc_pdu2_1E[1].dl_power_off     = (transmission_mode==5 ? 0 : 1);
  eNB2UE[0] = new_channel_desc_scm(eNB->frame_parms.nb_antennas_tx,
                                   UE->frame_parms.nb_antennas_rx,
                                   channel_model,
                                   N_RB2sampling_rate(eNB->frame_parms.N_RB_DL),
                                   N_RB2channel_bandwidth(eNB->frame_parms.N_RB_DL),
                                   forgetting_factor,
                                   rx_sample_offset,
                                   0, 0);

  if(num_rounds>1) {
    for(n=1; n<4; n++)
      eNB2UE[n] = new_channel_desc_scm(eNB->frame_parms.nb_antennas_tx,
                                       UE->frame_parms.nb_antennas_rx,
                                       channel_model,
                                       N_RB2sampling_rate(eNB->frame_parms.N_RB_DL),
                                       N_RB2channel_bandwidth(eNB->frame_parms.N_RB_DL),
                                       forgetting_factor,
                                       rx_sample_offset,
                                       0, 0);
  }

  if (eNB2UE[0]==NULL) {
    msg("Problem generating channel model. Exiting.\n");
    exit(-1);
  }

  if ((transmission_mode == 3) || (transmission_mode==4))
    Kmimo=2;
  else
    Kmimo=1;

  switch (ue_category) {
    case 1:
      Nsoft = 250368;
      break;

    case 2:
    case 3:
      Nsoft = 1237248;
      break;

    case 4:
      Nsoft = 1827072;
      break;

    default:
      printf("Unsupported UE category %d\n",ue_category);
      exit(-1);
      break;
  }

  for (k=0; k<n_users; k++) {
    // Create transport channel structures for 2 transport blocks (MIMO)
    for (i=0; i<2; i++) {
      eNB->dlsch[k][i] = new_eNB_dlsch(Kmimo,8,Nsoft,N_RB_DL,0,&eNB->frame_parms);

      if (!eNB->dlsch[k][i]) {
        printf("Can't get eNB dlsch structures\n");
        exit(-1);
      } else {
        // this initilisation may should be moved to another place
        for (j=0; j<4; j++) {
          for (aa=0; aa<n_tx_phy; aa++) {
            ue_spec_bf_weights = eNB->dlsch[k][i]->ue_spec_bf_weights[j][aa];

            for (re=0; re<OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES; re++) {
              if (n_tx_phy==1 || n_tx_phy==2)
                ue_spec_bf_weights[re] = 0x00007fff;
              else if (n_tx_phy==4 || n_tx_phy==8)
                ue_spec_bf_weights[re] = 0x00007fff>>1;
              else if (n_tx_phy==16)
                ue_spec_bf_weights[re] = 0x00007fff>>2;
              else if (n_tx_phy==64)
                ue_spec_bf_weights[re] = 0x00007fff>>4;
            }
          }
        }
      }

      eNB->dlsch[k][i]->rnti = n_rnti+k;
    }
  }

  for (i=0; i<2; i++) {
    UE->dlsch[UE->current_thread_id[subframe]][0][i]  = new_ue_dlsch(Kmimo,8,Nsoft,MAX_TURBO_ITERATIONS,N_RB_DL,0);

    if (!UE->dlsch[UE->current_thread_id[subframe]][0][i]) {
      printf("Can't get ue dlsch structures\n");
      exit(-1);
    }

    UE->dlsch[UE->current_thread_id[subframe]][0][i]->rnti   = n_rnti;
  }

  // structure for SIC at UE
  UE->dlsch_eNB[0] = new_eNB_dlsch(Kmimo,8,Nsoft,N_RB_DL,0,&eNB->frame_parms);

  if (DLSCH_alloc_pdu2_1E[0].tpmi == 5) {
    eNB->UE_stats[0].DL_pmi_single = (unsigned short)(taus()&0xffff);

    if (n_users>1)
      eNB->UE_stats[1].DL_pmi_single = (eNB->UE_stats[0].DL_pmi_single ^ 0x1555); //opposite PMI
  } else {
    eNB->UE_stats[0].DL_pmi_single = 0;

    if (n_users>1)
      eNB->UE_stats[1].DL_pmi_single = 0;
  }

  //TODO: allocate memory for calibration matrix and calib_dl_ch_estimates in init_lte.c
  //for first tests initialze calibration matrix with idendity
  //read_calibration_matrix(calib_fname, nb_ant, nb_freq, eNB->common_vars.tdd_calib_coeffs[0]);

  if (input_fd==NULL) {
    /*
    // common DCI
    memcpy(&dci_alloc[num_dci].dci_pdu[0],&CCCH_alloc_pdu,sizeof(DCI1A_5MHz_TDD_1_6_t));
    dci_alloc[num_dci].dci_length = sizeof_DCI1A_5MHz_TDD_1_6_t;
    dci_alloc[num_dci].L          = 2;
    dci_alloc[num_dci].rnti       = SI_RNTI;
    num_dci++;
    num_common_dci++;
    */

    // UE specific DCI
    for(k=0; k<n_users; k++) {
      switch(transmission_mode) {
        case 1:
        case 2:
        case 7:
          if (common_flag == 0) {
            if (eNB->frame_parms.frame_type == TDD) {
              switch (eNB->frame_parms.N_RB_DL) {
                case 6:
                  dci_length = sizeof_DCI1_1_5MHz_TDD_t;
                  dci_length_bytes = sizeof(DCI1_1_5MHz_TDD_t);
                  ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
                  ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                  ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                  ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                  ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 1;
                  ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;

                case 25:
                  dci_length = sizeof_DCI1_5MHz_TDD_t;
                  dci_length_bytes = sizeof(DCI1_5MHz_TDD_t);
                  ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
                  ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                  ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                  ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                  ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 1;
                  ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;

                case 50:
                  dci_length = sizeof_DCI1_10MHz_TDD_t;
                  dci_length_bytes = sizeof(DCI1_10MHz_TDD_t);
                  ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
                  ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                  ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                  ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                  ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 1;
                  ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;

                case 100:
                  ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
                  ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                  ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                  ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                  ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 1;
                  ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  dci_length = sizeof_DCI1_20MHz_TDD_t;
                  dci_length_bytes = sizeof(DCI1_20MHz_TDD_t);
                  break;
              }
            } else {
              switch (eNB->frame_parms.N_RB_DL) {
                case 6:
                  dci_length = sizeof_DCI1_1_5MHz_FDD_t;
                  dci_length_bytes = sizeof(DCI1_1_5MHz_FDD_t);
                  ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
                  ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                  ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                  ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 1;
                  ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;

                case 25:
                  dci_length = sizeof_DCI1_5MHz_FDD_t;
                  dci_length_bytes = sizeof(DCI1_5MHz_FDD_t);
                  ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
                  ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                  ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                  ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 1;
                  ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;

                case 50:
                  dci_length = sizeof_DCI1_10MHz_FDD_t;
                  dci_length_bytes = sizeof(DCI1_10MHz_FDD_t);
                  ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
                  ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                  ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                  ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 1;
                  ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;

                case 100:
                  dci_length = sizeof_DCI1_20MHz_FDD_t;
                  dci_length_bytes = sizeof(DCI1_20MHz_FDD_t);
                  ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
                  ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                  ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                  ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 1;
                  ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;
              }
            }

            memcpy(&dci_alloc[num_dci].dci_pdu[0],&DLSCH_alloc_pdu_1[k],dci_length_bytes);
            dci_alloc[num_dci].dci_length   = dci_length;
            dci_alloc[num_dci].L            = 1;
            dci_alloc[num_dci].rnti         = n_rnti+k;
            dci_alloc[num_dci].format       = format1;
            dci_alloc[num_dci].search_space = DCI_UE_SPACE;
            dump_dci(&eNB->frame_parms,&dci_alloc[num_dci]);
            printf("Generating dlsch params for user %d\n",k);
            generate_eNB_dlsch_params_from_dci(0,
                                               subframe,
                                               &DLSCH_alloc_pdu_1[0],
                                               n_rnti+k,
                                               format1,
                                               eNB->dlsch[0],
                                               &eNB->frame_parms,
                                               eNB->pdsch_config_dedicated,
                                               SI_RNTI,
                                               0,
                                               P_RNTI,
                                               eNB->UE_stats[0].DL_pmi_single,
                                               transmission_mode<7?0:transmission_mode);
            /*if (transmission_mode == 7)
            eNB->dlsch[0][0]->harq_processes[0]->mimo_mode = TM7; //Xiwen: to check about harq_pid*/
            num_dci++;
            num_ue_spec_dci++;
          } else {
            if (eNB->frame_parms.frame_type == TDD) {
              switch (eNB->frame_parms.N_RB_DL) {
                case 6:
                  dci_length = sizeof_DCI1A_1_5MHz_TDD_1_6_t;
                  dci_length_bytes = sizeof(DCI1A_1_5MHz_TDD_1_6_t);
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 0;
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;

                case 25:
                  dci_length = sizeof_DCI1A_5MHz_TDD_1_6_t;
                  dci_length_bytes = sizeof(DCI1A_5MHz_TDD_1_6_t);
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 0;
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rv              = 1;
                  break;

                case 50:
                  dci_length = sizeof_DCI1A_10MHz_TDD_1_6_t;
                  dci_length_bytes = sizeof(DCI1A_10MHz_TDD_1_6_t);
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 1;
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;

                case 100:
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 1;
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  dci_length = sizeof_DCI1A_20MHz_TDD_1_6_t;
                  dci_length_bytes = sizeof(DCI1A_20MHz_TDD_1_6_t);
                  break;
              }
            } else {
              switch (eNB->frame_parms.N_RB_DL) {
                case 6:
                  dci_length = sizeof_DCI1A_1_5MHz_FDD_t;
                  dci_length_bytes = sizeof(DCI1A_1_5MHz_FDD_t);
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 1;
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;

                case 25:
                  dci_length = sizeof_DCI1A_5MHz_FDD_t;
                  dci_length_bytes = sizeof(DCI1A_5MHz_FDD_t);
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 1;
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;

                case 50:
                  dci_length = sizeof_DCI1A_10MHz_FDD_t;
                  dci_length_bytes = sizeof(DCI1A_10MHz_FDD_t);
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 1;
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;

                case 100:
                  dci_length = sizeof_DCI1A_20MHz_FDD_t;
                  dci_length_bytes = sizeof(DCI1A_20MHz_FDD_t);
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 1;
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;
              }
            }

            memcpy(&dci_alloc[num_dci].dci_pdu[0],&DLSCH_alloc_pdu_1[k],dci_length_bytes);
            dci_alloc[num_dci].dci_length   = dci_length;
            dci_alloc[num_dci].L            = 1;
            dci_alloc[num_dci].rnti         = SI_RNTI;
            dci_alloc[num_dci].format       = format1A;
            dci_alloc[num_dci].firstCCE     = 0;
            dci_alloc[num_dci].search_space = DCI_COMMON_SPACE;
            dump_dci(&eNB->frame_parms,&dci_alloc[num_dci]);
            printf("Generating dlsch params for user %d\n",k);
            generate_eNB_dlsch_params_from_dci(0,
                                               subframe,
                                               &DLSCH_alloc_pdu_1[0],
                                               SI_RNTI,
                                               format1A,
                                               eNB->dlsch[0],
                                               &eNB->frame_parms,
                                               eNB->pdsch_config_dedicated,
                                               SI_RNTI,
                                               0,
                                               P_RNTI,
                                               eNB->UE_stats[0].DL_pmi_single,
                                               0);
            num_common_dci++;
            num_dci++;
          }

          break;

        case 3:
          if (common_flag == 0) {
            if (eNB->frame_parms.nb_antennas_tx == 2) {
              if (eNB->frame_parms.frame_type == TDD) {
                switch (eNB->frame_parms.N_RB_DL) {
                  case 6:
                    dci_length = sizeof_DCI2A_1_5MHz_2A_TDD_t;
                    dci_length_bytes = sizeof(DCI2A_1_5MHz_2A_TDD_t);
                    ((DCI2A_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                    ((DCI2A_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                    ((DCI2A_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                    ((DCI2A_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                    ((DCI2A_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs1             = mcs1;
                    ((DCI2A_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi1             = 1;
                    ((DCI2A_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv1              = 0;
                    ((DCI2A_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs2             = mcs2;
                    ((DCI2A_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi2             = 1;
                    ((DCI2A_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv2              = 0;
                    break;

                  case 25:
                    dci_length = sizeof_DCI2A_5MHz_2A_TDD_t;
                    dci_length_bytes = sizeof(DCI2A_5MHz_2A_TDD_t);
                    ((DCI2A_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
                    ((DCI2A_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                    ((DCI2A_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                    ((DCI2A_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                    ((DCI2A_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                    ((DCI2A_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs1             = mcs1;
                    ((DCI2A_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi1             = 1;
                    ((DCI2A_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv1              = 0;
                    ((DCI2A_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs2             = mcs2;
                    ((DCI2A_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi2             = 1;
                    ((DCI2A_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv2              = 0;
                    break;

                  case 50:
                    dci_length = sizeof_DCI2A_10MHz_2A_TDD_t;
                    dci_length_bytes = sizeof(DCI2A_10MHz_2A_TDD_t);
                    ((DCI2A_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
                    ((DCI2A_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                    ((DCI2A_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                    ((DCI2A_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                    ((DCI2A_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                    ((DCI2A_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs1             = mcs1;
                    ((DCI2A_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi1             = 1;
                    ((DCI2A_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv1              = 0;
                    ((DCI2A_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs2             = mcs2;
                    ((DCI2A_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi2             = 1;
                    ((DCI2A_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv2              = 0;
                    break;

                  case 100:
                    ((DCI2A_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
                    ((DCI2A_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                    ((DCI2A_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                    ((DCI2A_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                    ((DCI2A_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                    ((DCI2A_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs1             = mcs1;
                    ((DCI2A_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi1             = 1;
                    ((DCI2A_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv1              = 0;
                    ((DCI2A_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs2             = mcs2;
                    ((DCI2A_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi2             = 1;
                    ((DCI2A_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv2              = 0;
                    dci_length = sizeof_DCI2A_20MHz_2A_TDD_t;
                    dci_length_bytes = sizeof(DCI2A_20MHz_2A_TDD_t);
                    break;
                }
              } else {
                switch (eNB->frame_parms.N_RB_DL) {
                  case 6:
                    dci_length = sizeof_DCI2A_1_5MHz_2A_FDD_t;
                    dci_length_bytes = sizeof(DCI2A_1_5MHz_2A_FDD_t);
                    ((DCI2A_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                    ((DCI2A_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                    ((DCI2A_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                    ((DCI2A_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs1             = mcs1;
                    ((DCI2A_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi1             = 1;
                    ((DCI2A_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv1              = 0;
                    ((DCI2A_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs2             = mcs2;
                    ((DCI2A_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi2             = 1;
                    ((DCI2A_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv2              = 0;
                    break;

                  case 25:
                    dci_length = sizeof_DCI2A_5MHz_2A_FDD_t;
                    dci_length_bytes = sizeof(DCI2A_5MHz_2A_FDD_t);
                    ((DCI2A_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
                    ((DCI2A_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                    ((DCI2A_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                    ((DCI2A_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                    ((DCI2A_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs1             = mcs1;
                    ((DCI2A_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi1             = 1;
                    ((DCI2A_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv1              = 0;
                    ((DCI2A_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs2             = mcs2;
                    ((DCI2A_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi2             = 1;
                    ((DCI2A_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv2              = 0;
                    break;

                  case 50:
                    dci_length = sizeof_DCI2A_10MHz_2A_FDD_t;
                    dci_length_bytes = sizeof(DCI2A_10MHz_2A_FDD_t);
                    ((DCI2A_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
                    ((DCI2A_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                    ((DCI2A_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                    ((DCI2A_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                    ((DCI2A_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs1             = mcs1;
                    ((DCI2A_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi1             = 1;
                    ((DCI2A_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv1              = 0;
                    ((DCI2A_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs2             = mcs2;
                    ((DCI2A_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi2             = 1;
                    ((DCI2A_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv2              = 0;
                    break;

                  case 100:
                    dci_length = sizeof_DCI2A_20MHz_2A_FDD_t;
                    dci_length_bytes = sizeof(DCI2A_20MHz_2A_FDD_t);
                    ((DCI2A_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
                    ((DCI2A_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                    ((DCI2A_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                    ((DCI2A_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                    ((DCI2A_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs1             = mcs1;
                    ((DCI2A_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi1             = 1;
                    ((DCI2A_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv1              = 0;
                    ((DCI2A_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs2             = mcs2;
                    ((DCI2A_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi2             = 1;
                    ((DCI2A_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv2              = 0;
                    break;
                }
              }
            } else if (eNB->frame_parms.nb_antennas_tx == 4) {
            }

            memcpy(&dci_alloc[num_dci].dci_pdu[0],&DLSCH_alloc_pdu_1[k],dci_length_bytes);
            dci_alloc[num_dci].dci_length   = dci_length;
            dci_alloc[num_dci].L            = 1;
            dci_alloc[num_dci].rnti         = n_rnti+k;
            dci_alloc[num_dci].format       = format2A;
            dci_alloc[num_dci].search_space = DCI_UE_SPACE;
            dump_dci(&eNB->frame_parms,&dci_alloc[num_dci]);
            printf("Generating dlsch params for user %d / format 2A (%d)\n",k,format2A);
            generate_eNB_dlsch_params_from_dci(0,
                                               subframe,
                                               &DLSCH_alloc_pdu_1[0],
                                               n_rnti+k,
                                               format2A,
                                               eNB->dlsch[0],
                                               &eNB->frame_parms,
                                               eNB->pdsch_config_dedicated,
                                               SI_RNTI,
                                               0,
                                               P_RNTI,
                                               eNB->UE_stats[0].DL_pmi_single,
                                               0);
            num_dci++;
            num_ue_spec_dci++;
          } else {
            if (eNB->frame_parms.frame_type == TDD) {
              switch (eNB->frame_parms.N_RB_DL) {
                case 6:
                  dci_length = sizeof_DCI1A_1_5MHz_TDD_1_6_t;
                  dci_length_bytes = sizeof(DCI1A_1_5MHz_TDD_1_6_t);
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 0;
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;

                case 25:
                  dci_length = sizeof_DCI1A_5MHz_TDD_1_6_t;
                  dci_length_bytes = sizeof(DCI1A_5MHz_TDD_1_6_t);
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 0;
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rv              = 1;
                  break;

                case 50:
                  dci_length = sizeof_DCI1A_10MHz_TDD_1_6_t;
                  dci_length_bytes = sizeof(DCI1A_10MHz_TDD_1_6_t);
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 1;
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;

                case 100:
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 1;
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  dci_length = sizeof_DCI1A_20MHz_TDD_1_6_t;
                  dci_length_bytes = sizeof(DCI1A_20MHz_TDD_1_6_t);
                  break;
              }
            } else {
              switch (eNB->frame_parms.N_RB_DL) {
                case 6:
                  dci_length = sizeof_DCI1A_1_5MHz_FDD_t;
                  dci_length_bytes = sizeof(DCI1A_1_5MHz_FDD_t);
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 1;
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;

                case 25:
                  dci_length = sizeof_DCI1A_5MHz_FDD_t;
                  dci_length_bytes = sizeof(DCI1A_5MHz_FDD_t);
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 1;
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;

                case 50:
                  dci_length = sizeof_DCI1A_10MHz_FDD_t;
                  dci_length_bytes = sizeof(DCI1A_10MHz_FDD_t);
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 1;
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;

                case 100:
                  dci_length = sizeof_DCI1A_20MHz_FDD_t;
                  dci_length_bytes = sizeof(DCI1A_20MHz_FDD_t);
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 1;
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;
              }
            }

            memcpy(&dci_alloc[num_dci].dci_pdu[0],&DLSCH_alloc_pdu_1[k],dci_length_bytes);
            dci_alloc[num_dci].dci_length   = dci_length;
            dci_alloc[num_dci].L            = 1;
            dci_alloc[num_dci].rnti         = SI_RNTI;
            dci_alloc[num_dci].format       = format1A;
            dci_alloc[num_dci].firstCCE     = 0;
            dci_alloc[num_dci].search_space = DCI_COMMON_SPACE;
            dump_dci(&eNB->frame_parms,&dci_alloc[num_dci]);
            printf("Generating dlsch params for user %d\n",k);
            generate_eNB_dlsch_params_from_dci(0,
                                               subframe,
                                               &DLSCH_alloc_pdu_1[0],
                                               SI_RNTI,
                                               format1A,
                                               eNB->dlsch[0],
                                               &eNB->frame_parms,
                                               eNB->pdsch_config_dedicated,
                                               SI_RNTI,
                                               0,
                                               P_RNTI,
                                               eNB->UE_stats[0].DL_pmi_single,
                                               0);
            num_common_dci++;
            num_dci++;
          }

          printf("Generated DCI format 2A (Transmission Mode 3)\n");
          break;

        case 4:
          if (common_flag == 0) {
            if (eNB->frame_parms.nb_antennas_tx == 2) {
              if (eNB->frame_parms.frame_type == TDD) {
                switch (eNB->frame_parms.N_RB_DL) {
                  case 6:
                    dci_length = sizeof_DCI2_1_5MHz_2A_TDD_t;
                    dci_length_bytes = sizeof(DCI2_1_5MHz_2A_TDD_t);
                    ((DCI2_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                    ((DCI2_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                    ((DCI2_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                    ((DCI2_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                    ((DCI2_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs1             = mcs1;
                    ((DCI2_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi1             = 1;
                    ((DCI2_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv1              = 0;
                    ((DCI2_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs2             = mcs2;
                    ((DCI2_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi2             = 1;
                    ((DCI2_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv2              = 0;
                    break;

                  case 25:
                    dci_length = sizeof_DCI2_5MHz_2A_TDD_t;
                    dci_length_bytes = sizeof(DCI2_5MHz_2A_TDD_t);
                    ((DCI2_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
                    ((DCI2_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                    ((DCI2_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                    ((DCI2_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                    ((DCI2_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                    ((DCI2_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs1             = mcs1;
                    ((DCI2_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi1             = 1;
                    ((DCI2_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv1              = 0;
                    ((DCI2_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs2             = mcs2;
                    ((DCI2_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi2             = 1;
                    ((DCI2_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv2              = 0;
                    break;

                  case 50:
                    dci_length = sizeof_DCI2_10MHz_2A_TDD_t;
                    dci_length_bytes = sizeof(DCI2_10MHz_2A_TDD_t);
                    ((DCI2_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
                    ((DCI2_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                    ((DCI2_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                    ((DCI2_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                    ((DCI2_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                    ((DCI2_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs1             = mcs1;
                    ((DCI2_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi1             = 1;
                    ((DCI2_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv1              = 0;
                    ((DCI2_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs2             = mcs2;
                    ((DCI2_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi2             = 1;
                    ((DCI2_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv2              = 0;
                    break;

                  case 100:
                    ((DCI2_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
                    ((DCI2_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                    ((DCI2_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                    ((DCI2_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                    ((DCI2_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                    ((DCI2_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs1             = mcs1;
                    ((DCI2_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi1             = 1;
                    ((DCI2_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv1              = 0;
                    ((DCI2_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->mcs2             = mcs2;
                    ((DCI2_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->ndi2             = 1;
                    ((DCI2_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[k])->rv2              = 0;
                    dci_length = sizeof_DCI2_20MHz_2A_TDD_t;
                    dci_length_bytes = sizeof(DCI2_20MHz_2A_TDD_t);
                    break;
                }
              } else {
                switch (eNB->frame_parms.N_RB_DL) {
                  case 6:
                    dci_length = sizeof_DCI2_1_5MHz_2A_FDD_t;
                    dci_length_bytes = sizeof(DCI2_1_5MHz_2A_FDD_t);
                    ((DCI2_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                    ((DCI2_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                    ((DCI2_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                    ((DCI2_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs1             = mcs1;
                    ((DCI2_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi1             = 1;
                    ((DCI2_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv1              = 0;
                    ((DCI2_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs2             = mcs2;
                    ((DCI2_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi2             = 1;
                    ((DCI2_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv2              = 0;
                    break;

                  case 25:
                    dci_length = sizeof_DCI2_5MHz_2A_FDD_t;
                    dci_length_bytes = sizeof(DCI2_5MHz_2A_FDD_t);
                    ((DCI2_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
                    ((DCI2_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                    ((DCI2_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                    ((DCI2_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                    ((DCI2_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs1             = mcs1;
                    ((DCI2_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi1             = 1;
                    ((DCI2_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv1              = 0;
                    ((DCI2_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs2             = mcs2;
                    ((DCI2_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi2             = 1;
                    ((DCI2_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv2              = 0;
                    break;

                  case 50:
                    dci_length = sizeof_DCI2_10MHz_2A_FDD_t;
                    dci_length_bytes = sizeof(DCI2_10MHz_2A_FDD_t);
                    ((DCI2_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
                    ((DCI2_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                    ((DCI2_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                    ((DCI2_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                    ((DCI2_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs1             = mcs1;
                    ((DCI2_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi1             = 1;
                    ((DCI2_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv1              = 0;
                    ((DCI2_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs2             = mcs2;
                    ((DCI2_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi2             = 1;
                    ((DCI2_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv2              = 0;
                    break;

                  case 100:
                    dci_length = sizeof_DCI2_20MHz_2A_FDD_t;
                    dci_length_bytes = sizeof(DCI2_20MHz_2A_FDD_t);
                    ((DCI2_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rah              = 0;
                    ((DCI2_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = DLSCH_RB_ALLOC;
                    ((DCI2_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = 0;
                    ((DCI2_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                    ((DCI2_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs1             = mcs1;
                    ((DCI2_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi1             = 1;
                    ((DCI2_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv1              = 0;
                    ((DCI2_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs2             = mcs2;
                    ((DCI2_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi2             = 1;
                    ((DCI2_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv2              = 0;
                    break;
                }
              }
            } else if (eNB->frame_parms.nb_antennas_tx == 4) {
            }

            memcpy(&dci_alloc[num_dci].dci_pdu[0],&DLSCH_alloc_pdu_1[k],dci_length_bytes);
            dci_alloc[num_dci].dci_length   = dci_length;
            dci_alloc[num_dci].L            = 1;
            dci_alloc[num_dci].rnti         = n_rnti+k;
            dci_alloc[num_dci].format       = format2;
            dci_alloc[num_dci].search_space = DCI_UE_SPACE;
            dump_dci(&eNB->frame_parms,&dci_alloc[num_dci]);
            printf("Generating dlsch params for user %d\n",k);
            generate_eNB_dlsch_params_from_dci(0,
                                               subframe,
                                               &DLSCH_alloc_pdu_1[0],
                                               n_rnti+k,
                                               format2,
                                               eNB->dlsch[0],
                                               &eNB->frame_parms,
                                               eNB->pdsch_config_dedicated,
                                               SI_RNTI,
                                               0,
                                               P_RNTI,
                                               eNB->UE_stats[0].DL_pmi_single,
                                               0);
            num_dci++;
            num_ue_spec_dci++;
          } else {
            if (eNB->frame_parms.frame_type == TDD) {
              switch (eNB->frame_parms.N_RB_DL) {
                case 6:
                  dci_length = sizeof_DCI1A_1_5MHz_TDD_1_6_t;
                  dci_length_bytes = sizeof(DCI1A_1_5MHz_TDD_1_6_t);
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 0;
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_1_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;

                case 25:
                  dci_length = sizeof_DCI1A_5MHz_TDD_1_6_t;
                  dci_length_bytes = sizeof(DCI1A_5MHz_TDD_1_6_t);
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 0;
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_5MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rv              = 1;
                  break;

                case 50:
                  dci_length = sizeof_DCI1A_10MHz_TDD_1_6_t;
                  dci_length_bytes = sizeof(DCI1A_10MHz_TDD_1_6_t);
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 1;
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_10MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;

                case 100:
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 1;
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->dai              = 0;
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_20MHz_TDD_1_6_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  dci_length = sizeof_DCI1A_20MHz_TDD_1_6_t;
                  dci_length_bytes = sizeof(DCI1A_20MHz_TDD_1_6_t);
                  break;
              }
            } else {
              switch (eNB->frame_parms.N_RB_DL) {
                case 6:
                  dci_length = sizeof_DCI1A_1_5MHz_FDD_t;
                  dci_length_bytes = sizeof(DCI1A_1_5MHz_FDD_t);
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 1;
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;

                case 25:
                  dci_length = sizeof_DCI1A_5MHz_FDD_t;
                  dci_length_bytes = sizeof(DCI1A_5MHz_FDD_t);
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 1;
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;

                case 50:
                  dci_length = sizeof_DCI1A_10MHz_FDD_t;
                  dci_length_bytes = sizeof(DCI1A_10MHz_FDD_t);
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 1;
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;

                case 100:
                  dci_length = sizeof_DCI1A_20MHz_FDD_t;
                  dci_length_bytes = sizeof(DCI1A_20MHz_FDD_t);
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->type             = 1;
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->vrb_type         = 1;
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rballoc          = computeRIV(eNB->frame_parms.N_RB_DL,0,9);
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->TPC              = TPC;
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->harq_pid         = 0;
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->mcs             = mcs1;
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->ndi             = 0;
                  ((DCI1A_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[k])->rv              = 0;
                  break;
              }
            }

            memcpy(&dci_alloc[num_dci].dci_pdu[0],&DLSCH_alloc_pdu_1[k],dci_length_bytes);
            dci_alloc[num_dci].dci_length   = dci_length;
            dci_alloc[num_dci].L            = 1;
            dci_alloc[num_dci].rnti         = SI_RNTI;
            dci_alloc[num_dci].format       = format1A;
            dci_alloc[num_dci].firstCCE     = 0;
            dci_alloc[num_dci].search_space = DCI_COMMON_SPACE;
            dump_dci(&eNB->frame_parms,&dci_alloc[num_dci]);
            printf("Generating dlsch params for user %d\n",k);
            generate_eNB_dlsch_params_from_dci(0,
                                               subframe,
                                               &DLSCH_alloc_pdu_1[0],
                                               SI_RNTI,
                                               format1A,
                                               eNB->dlsch[0],
                                               &eNB->frame_parms,
                                               eNB->pdsch_config_dedicated,
                                               SI_RNTI,
                                               0,
                                               P_RNTI,
                                               eNB->UE_stats[0].DL_pmi_single,
                                               0);
            num_common_dci++;
            num_dci++;
          }

          break;

        case 5:
        case 6:
          memcpy(&dci_alloc[num_dci].dci_pdu[0],&DLSCH_alloc_pdu2_1E[k],sizeof(DCI1E_5MHz_2A_M10PRB_TDD_t));
          dci_alloc[num_dci].dci_length   = sizeof_DCI1E_5MHz_2A_M10PRB_TDD_t;
          dci_alloc[num_dci].L            = 1;
          dci_alloc[num_dci].rnti         = n_rnti+k;
          dci_alloc[num_dci].format       = format1E_2A_M10PRB;
          dci_alloc[num_dci].firstCCE     = 4*k;
          dci_alloc[num_dci].search_space = DCI_UE_SPACE;
          printf("Generating dlsch params for user %d\n",k);
          generate_eNB_dlsch_params_from_dci(0,
                                             subframe,
                                             &DLSCH_alloc_pdu2_1E[k],
                                             n_rnti+k,
                                             format1E_2A_M10PRB,
                                             eNB->dlsch[k],
                                             &eNB->frame_parms,
                                             eNB->pdsch_config_dedicated,
                                             SI_RNTI,
                                             0,
                                             P_RNTI,
                                             eNB->UE_stats[k].DL_pmi_single,
                                             0);
          dump_dci(&eNB->frame_parms,&dci_alloc[num_dci]);
          num_ue_spec_dci++;
          num_dci++;
          break;

        default:
          printf("Unsupported Transmission Mode!!!");
          exit(-1);
          break;
      }

      /*
      memcpy(&dci_alloc[1].dci_pdu[0],&UL_alloc_pdu,sizeof(DCI0_5MHz_TDD0_t));
      dci_alloc[1].dci_length = sizeof_DCI0_5MHz_TDD_0_t;
      dci_alloc[1].L          = 2;
      dci_alloc[1].rnti       = n_rnti;
      */
    }

    if (n_frames==1) printf("num_pdcch_symbols %d, numCCE %d => ",num_pdcch_symbols,numCCE);

    numCCE = get_nCCE(num_pdcch_symbols,&eNB->frame_parms,get_mi(&eNB->frame_parms,subframe));

    if (n_frames==1) printf("%d\n",numCCE);

    // apply RNTI-based nCCE allocation
    memset(CCE_table,0,800*sizeof(int));

    for (i=num_common_dci; i<num_dci; i++) {
      dci_alloc[i].firstCCE = get_nCCE_offset_l1(CCE_table,
                              1<<dci_alloc[i].L,
                              numCCE,
                              (dci_alloc[i].rnti==SI_RNTI)? 1 : 0,
                              dci_alloc[i].rnti,
                              subframe);

      if (n_frames==1)
        printf("dci %d: rnti %x, format %d : nCCE %d/%d\n",i,dci_alloc[i].rnti, dci_alloc[i].format,
               dci_alloc[i].firstCCE,numCCE);
    }

    for (k=0; k<n_users; k++) {
      input_buffer_length0 = eNB->dlsch[k][0]->harq_processes[0]->TBS/8;
      input_buffer0[k] = (unsigned char *)malloc(input_buffer_length0+4);
      memset(input_buffer0[k],0,input_buffer_length0+4);
      input_buffer_length1 = eNB->dlsch[k][1]->harq_processes[0]->TBS/8;
      input_buffer1[k] = (unsigned char *)malloc(input_buffer_length1+4);
      memset(input_buffer1[k],0,input_buffer_length1+4);

      if (input_trch_file==0) {
        for (i=0; i<input_buffer_length0; i++) {
          //input_buffer0[k][i] = (unsigned char)(i&0xff);
          input_buffer0[k][i] = (unsigned char)(taus()&0xff);
        }

        for (i=0; i<input_buffer_length1; i++) {
          input_buffer1[k][i]= (unsigned char)(taus()&0xff);
        }
      } else {
        i=0;

        while ((!feof(input_trch_fd)) && (i<input_buffer_length0<<3)) {
          ret=fscanf(input_trch_fd,"%15s",input_trch_val);

          if (input_trch_val[0] == '1')
            input_buffer0[k][i>>3]+=(1<<(7-(i&7)));

          if (i<16)
            printf("input_trch_val %d : %c\n",i,input_trch_val[0]);

          i++;

          if (((i%8) == 0) && (i<17))
            printf("%x\n",input_buffer0[k][(i-1)>>3]);
        }

        printf("Read in %d bits\n",i);
      }
    }
  }

  // this is for user 0 only
  coded_bits_per_codeword = get_G(&eNB->frame_parms,
                                  eNB->dlsch[0][0]->harq_processes[0]->nb_rb,
                                  eNB->dlsch[0][0]->harq_processes[0]->rb_alloc,
                                  get_Qm(eNB->dlsch[0][0]->harq_processes[0]->mcs),
                                  eNB->dlsch[0][0]->harq_processes[0]->Nl,
                                  num_pdcch_symbols,
                                  0,
                                  subframe,
                                  (transmission_mode<7?0:transmission_mode));
  uncoded_ber_bit = (short *) malloc(sizeof(short)*coded_bits_per_codeword);
  printf("uncoded_ber_bit=%p\n",uncoded_ber_bit);
  snr_step = input_snr_step;
  UE->high_speed_flag = 0; //1
  UE->ch_est_alpha=0;

  for (ch_realization=0; ch_realization<n_ch_rlz; ch_realization++) {
    if(abstx) {
      printf("**********************Channel Realization Index = %d **************************\n", ch_realization);
    }

    for (SNR=snr0; SNR<snr1; SNR+=snr_step) {
      UE->proc.proc_rxtx[UE->current_thread_id[subframe]].frame_rx=0;
      errs[0]=0;
      errs[1]=0;
      errs[2]=0;
      errs[3]=0;
      errs2[0]=0;
      errs2[1]=0;
      errs2[2]=0;
      errs2[3]=0;
      round_trials[0] = 0;
      round_trials[1] = 0;
      round_trials[2] = 0;
      round_trials[3] = 0;
      dci_errors=0;
      //      avg_ber = 0;
      round=0;
      avg_iter = 0;
      iter_trials=0;
      reset_meas(&eNB->phy_proc_tx); // total eNB tx
      reset_meas(&eNB->dlsch_scrambling_stats);
      reset_meas(&UE->dlsch_unscrambling_stats);
      reset_meas(&eNB->ofdm_mod_stats);
      reset_meas(&eNB->dlsch_modulation_stats);
      reset_meas(&eNB->dlsch_encoding_stats);
      reset_meas(&eNB->dlsch_interleaving_stats);
      reset_meas(&eNB->dlsch_rate_matching_stats);
      reset_meas(&eNB->dlsch_turbo_encoding_stats);
      reset_meas(&UE->phy_proc_rx[UE->current_thread_id[subframe]]); // total UE rx
      reset_meas(&UE->ofdm_demod_stats);
      reset_meas(&UE->dlsch_channel_estimation_stats);
      reset_meas(&UE->dlsch_freq_offset_estimation_stats);
      reset_meas(&UE->rx_dft_stats);
      reset_meas(&UE->dlsch_decoding_stats);
      reset_meas(&UE->dlsch_turbo_decoding_stats);
      reset_meas(&UE->dlsch_deinterleaving_stats);
      reset_meas(&UE->dlsch_rate_unmatching_stats);
      reset_meas(&UE->dlsch_tc_init_stats);
      reset_meas(&UE->dlsch_tc_alpha_stats);
      reset_meas(&UE->dlsch_tc_beta_stats);
      reset_meas(&UE->dlsch_tc_gamma_stats);
      reset_meas(&UE->dlsch_tc_ext_stats);
      reset_meas(&UE->dlsch_tc_intl1_stats);
      reset_meas(&UE->dlsch_tc_intl2_stats);
      // initialization
      struct list time_vector_tx;
      initialize(&time_vector_tx);
      struct list time_vector_tx_ifft;
      initialize(&time_vector_tx_ifft);
      struct list time_vector_tx_mod;
      initialize(&time_vector_tx_mod);
      struct list time_vector_tx_enc;
      initialize(&time_vector_tx_enc);
      struct list time_vector_rx;
      initialize(&time_vector_rx);
      struct list time_vector_rx_fft;
      initialize(&time_vector_rx_fft);
      struct list time_vector_rx_demod;
      initialize(&time_vector_rx_demod);
      struct list time_vector_rx_dec;
      initialize(&time_vector_rx_dec);

      for (trials = 0; trials<n_frames; trials++) {
        //  printf("Trial %d\n",trials);
        fflush(stdout);
        round=0;
        //if (trials%100==0)
        eNB2UE[0]->first_run = 1;
        ret = UE->dlsch[UE->current_thread_id[subframe]][0][0]->max_turbo_iterations+1;

        while ((round < num_rounds) && (ret > UE->dlsch[UE->current_thread_id[subframe]][0][0]->max_turbo_iterations)) {
          //printf("Trial %d, round %d\n",trials,round);
          round_trials[round]++;

          if(transmission_mode>=5&&transmission_mode<7)
            pmi_feedback=1;
          else
            pmi_feedback=0;

          if (abstx) {
            if (trials==0 && round==0 && SNR==snr0)  //generate a new channel
              hold_channel = 0;
            else
              hold_channel = 1;
          } else
            hold_channel = 0;//(round==0) ? 0 : 1;

PMI_FEEDBACK:

          //make sure dlsim is called with perfect channel estimation option (for freq_channel)
          //fill drs_ch_estimates with data from eNB2UE->chF
          for(aa=0; aa<frame_parms->nb_antenna_ports_eNB; aa++) {
            for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
              for (i=0; i<frame_parms->N_RB_DL*12; i++) {
                for (l=0; l<frame_parms->symbols_per_tti; l++) {
                  ((int16_t *) eNB->pusch_vars[0]->drs_ch_estimates[0][(aa<<1)+aarx])[2*i+(l*frame_parms->ofdm_symbol_size)*2]=(int16_t)(eNB2UE[round]->chF[aarx+(aa*frame_parms->nb_antennas_rx)][i].x*AMP);
                  //printf("x=%d,AMP=%d\n",eNB2UE[round]->chF[aarx+(aa*frame_parms->nb_antennas_rx)][i].x,AMP);
                  ((int16_t *) eNB->pusch_vars[0]->drs_ch_estimates[0][(aa<<1)+aarx])[2*i+1+(l*frame_parms->ofdm_symbol_size)*2]=(int16_t)(eNB2UE[round]->chF[aarx+(aa*frame_parms->nb_antennas_rx)][i].y*AMP);
                }
              }
            }
          }

          estimate_DLCSI_from_ULCSI(eNB->dlsch[0][0]->calib_dl_ch_estimates,
                                    &eNB->pusch_vars[0]->drs_ch_estimates[0][0/*position of second DMRS*/],
                                    eNB->common_vars.tdd_calib_coeffs[0],
                                    frame_parms->nb_antennas_tx,
                                    frame_parms->N_RB_DL*12);
          compute_BF_weights(eNB->dlsch[0][0]->ue_spec_bf_weights[0],
                             eNB->dlsch[0][0]->calib_dl_ch_estimates,
                             MRT,
                             frame_parms->nb_antennas_tx,
                             frame_parms->N_RB_DL*12);

          //printf("Trial %d : Round %d, pmi_feedback %d \n",trials,round,pmi_feedback);
          for (aa=0; aa<NB_ANTENNA_PORTS_ENB; aa++) {
            memset(&eNB->common_vars.txdataF[eNB_id][aa][0],0,FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX*sizeof(int32_t));
          }

          if (input_fd==NULL) {
            start_meas(&eNB->phy_proc_tx);

            // Simulate HARQ procedures!!!
            if (common_flag == 0) {
              if (round == 0) {   // First round
                TB0_active = 1;
                eNB->dlsch[0][0]->harq_processes[0]->rvidx = round&3;

                if (eNB->frame_parms.frame_type == TDD) {
                  switch (transmission_mode) {
                    case 1:
                    case 2:
                    case 7:
                      switch (eNB->frame_parms.N_RB_DL) {
                        case 6:
                          ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi             = trials&1;
                          ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv              = 0;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI1_1_5MHz_TDD_t));
                          break;

                        case 25:
                          ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi             = trials&1;
                          ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv              = 0;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI1_5MHz_TDD_t));
                          break;

                        case 50:
                          ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi             = trials&1;
                          ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv              = 0;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI1_10MHz_TDD_t));
                          break;

                        case 100:
                          ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi             = trials&1;
                          ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv              = 0;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI1_20MHz_TDD_t));
                          break;
                      }

                      break;

                    case 3:
                      switch (eNB->frame_parms.N_RB_DL) {
                        case 6:
                          ((DCI2A_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi1             = trials&1;
                          ((DCI2A_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = 0;
                          ((DCI2A_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                          ((DCI2A_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = 0;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI2A_1_5MHz_2A_TDD_t));
                          break;

                        case 25:
                          ((DCI2A_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi1             = trials&1;
                          ((DCI2A_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = 0;
                          ((DCI2A_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                          ((DCI2A_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = 0;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI2A_5MHz_2A_TDD_t));
                          break;

                        case 50:
                          ((DCI2A_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi1             = trials&1;
                          ((DCI2A_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = 0;
                          ((DCI2A_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                          ((DCI2A_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = 0;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI2A_10MHz_2A_TDD_t));
                          break;

                        case 100:
                          ((DCI2A_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi1             = trials&1;
                          ((DCI2A_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = 0;
                          ((DCI2A_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                          ((DCI2A_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = 0;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI2A_20MHz_2A_TDD_t));
                          break;
                      }

                      break;

                    case 5:
                      DLSCH_alloc_pdu2_1E[0].ndi             = trials&1;
                      DLSCH_alloc_pdu2_1E[0].rv              = 0;
                      memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu2_1E[0],sizeof(DCI1E_5MHz_2A_M10PRB_TDD_t));
                      break;
                  }
                } else { // FDD
                  switch (transmission_mode) {
                    case 1:
                    case 2:
                    case 7:
                      switch (eNB->frame_parms.N_RB_DL) {
                        case 6:
                          ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi             = trials&1;
                          ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv              = 0;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI1_1_5MHz_FDD_t));
                          break;

                        case 25:
                          ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi             = trials&1;
                          ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv              = 0;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI1_5MHz_FDD_t));
                          break;

                        case 50:
                          ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi             = trials&1;
                          ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv              = 0;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI1_10MHz_FDD_t));
                          break;

                        case 100:
                          ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi             = trials&1;
                          ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv              = 0;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI1_20MHz_FDD_t));
                          break;
                      }

                      break;

                    case 3:
                      switch (eNB->frame_parms.N_RB_DL) {
                        case 6:
                          ((DCI2A_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi1             = trials&1;
                          ((DCI2A_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = 0;
                          ((DCI2A_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                          ((DCI2A_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = 0;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI2A_1_5MHz_2A_FDD_t));
                          break;

                        case 25:
                          ((DCI2A_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi1             = trials&1;
                          ((DCI2A_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = 0;
                          ((DCI2A_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                          ((DCI2A_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = 0;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI2A_5MHz_2A_FDD_t));
                          break;

                        case 50:
                          ((DCI2A_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi1             = trials&1;
                          ((DCI2A_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = 0;
                          ((DCI2A_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                          ((DCI2A_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = 0;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI2A_10MHz_2A_FDD_t));
                          break;

                        case 100:
                          ((DCI2A_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi1             = trials&1;
                          ((DCI2A_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = 0;
                          ((DCI2A_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                          ((DCI2A_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = 0;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI2A_20MHz_2A_FDD_t));
                          break;
                      }

                      break;

                    case 5:
                      DLSCH_alloc_pdu2_1E[0].ndi             = trials&1;
                      DLSCH_alloc_pdu2_1E[0].rv              = 0;
                      memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu2_1E[0],sizeof(DCI1E_5MHz_2A_M10PRB_TDD_t));
                      break;
                  }
                }
              } else {
                eNB->dlsch[0][0]->harq_processes[0]->rvidx = round&3;

                if (eNB->frame_parms.frame_type == TDD) {
                  switch (transmission_mode) {
                    case 1:
                    case 2:
                    case 7:
                      switch (eNB->frame_parms.N_RB_DL) {
                        case 6:
                          ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi             = trials&1;
                          ((DCI1_1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv              = round&3;;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI1_1_5MHz_TDD_t));
                          break;

                        case 25:
                          ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi             = trials&1;
                          ((DCI1_5MHz_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv              = round&3;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI1_5MHz_TDD_t));
                          break;

                        case 50:
                          ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi             = trials&1;
                          ((DCI1_10MHz_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv              = round&3;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI1_10MHz_TDD_t));
                          break;

                        case 100:
                          ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi             = trials&1;
                          ((DCI1_20MHz_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv              = round&3;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI1_20MHz_TDD_t));
                          break;
                      }

                      break;

                    case 3:
                      switch (eNB->frame_parms.N_RB_DL) {
                        case 6:
                          if (TB0_active==1) {
                            ((DCI2A_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi1             = trials&1;
                            ((DCI2A_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = round&3;
                            ((DCI2A_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                            ((DCI2A_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = round&3;
                          } else { // deactivate TB0
                            ((DCI2A_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->mcs1             = 0;
                            ((DCI2A_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = 1;
                            ((DCI2A_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                            ((DCI2A_1_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = round&3;
                          }

                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI2A_1_5MHz_2A_TDD_t));
                          break;

                        case 25:
                          if (TB0_active==1) {
                            ((DCI2A_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi1             = trials&1;
                            ((DCI2A_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = round&3;
                            ((DCI2A_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                            ((DCI2A_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = round&3;
                          } else { // deactivate TB0
                            ((DCI2A_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->mcs1             = 0;
                            ((DCI2A_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = 1;
                            ((DCI2A_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                            ((DCI2A_5MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = round&3;
                          }

                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI2A_5MHz_2A_TDD_t));
                          break;

                        case 50:
                          if (TB0_active==1) {
                            ((DCI2A_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi1             = trials&1;
                            ((DCI2A_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = round&3;
                            ((DCI2A_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                            ((DCI2A_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = round&3;
                          } else { // deactivate TB0
                            ((DCI2A_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->mcs1             = 0;
                            ((DCI2A_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = 1;
                            ((DCI2A_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                            ((DCI2A_10MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = round&3;
                          }

                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI2A_10MHz_2A_TDD_t));
                          break;

                        case 100:
                          if (TB0_active==1) {
                            ((DCI2A_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi1             = trials&1;
                            ((DCI2A_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = round&3;
                            ((DCI2A_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                            ((DCI2A_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = round&3;
                          } else { // deactivate TB0
                            ((DCI2A_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->mcs1             = 0;
                            ((DCI2A_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = 1;
                            ((DCI2A_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                            ((DCI2A_20MHz_2A_TDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = round&3;
                          }

                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI2A_20MHz_2A_TDD_t));
                          break;
                      }

                      break;

                    case 5:
                      DLSCH_alloc_pdu2_1E[0].ndi             = trials&1;
                      DLSCH_alloc_pdu2_1E[0].rv              = round&3;
                      memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu2_1E[0],sizeof(DCI1E_5MHz_2A_M10PRB_TDD_t));
                      break;
                  }
                } else {
                  switch (transmission_mode) {
                    case 1:
                    case 2:
                    case 7:
                      switch (eNB->frame_parms.N_RB_DL) {
                        case 6:
                          ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi             = trials&1;
                          ((DCI1_1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv              = round&3;;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI1_1_5MHz_FDD_t));
                          break;

                        case 25:
                          ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi             = trials&1;
                          ((DCI1_5MHz_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv              = round&3;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI1_5MHz_FDD_t));
                          break;

                        case 50:
                          ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi             = trials&1;
                          ((DCI1_10MHz_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv              = round&3;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI1_10MHz_FDD_t));
                          break;

                        case 100:
                          ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi             = trials&1;
                          ((DCI1_20MHz_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv              = round&3;
                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI1_20MHz_FDD_t));
                          break;
                      }

                      break;

                    case 3:
                      switch (eNB->frame_parms.N_RB_DL) {
                        case 6:
                          if (TB0_active==1) {
                            ((DCI2A_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi1             = trials&1;
                            ((DCI2A_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = round&3;
                            ((DCI2A_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                            ((DCI2A_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = round&3;
                          } else { // deactivate TB0
                            ((DCI2A_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->mcs1             = 0;
                            ((DCI2A_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = 1;
                            ((DCI2A_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                            ((DCI2A_1_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = round&3;
                          }

                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI2A_1_5MHz_2A_FDD_t));
                          break;

                        case 25:
                          if (TB0_active==1) {
                            ((DCI2A_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi1             = trials&1;
                            ((DCI2A_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = round&3;
                            ((DCI2A_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                            ((DCI2A_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = round&3;
                          } else { // deactivate TB0
                            ((DCI2A_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->mcs1             = 0;
                            ((DCI2A_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = 1;
                            ((DCI2A_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                            ((DCI2A_5MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = round&3;
                          }

                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI2A_5MHz_2A_FDD_t));
                          break;

                        case 50:
                          if (TB0_active==1) {
                            ((DCI2A_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi1             = trials&1;
                            ((DCI2A_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = round&3;
                            ((DCI2A_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                            ((DCI2A_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = round&3;
                          } else { // deactivate TB0
                            ((DCI2A_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->mcs1             = 0;
                            ((DCI2A_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = 1;
                            ((DCI2A_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                            ((DCI2A_10MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = round&3;
                          }

                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI2A_10MHz_2A_FDD_t));
                          break;

                        case 100:
                          if (TB0_active==1) {
                            ((DCI2A_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi1             = trials&1;
                            ((DCI2A_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = round&3;
                            ((DCI2A_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                            ((DCI2A_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = round&3;
                          } else { // deactivate TB0
                            ((DCI2A_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->mcs1             = 0;
                            ((DCI2A_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv1              = 1;
                            ((DCI2A_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->ndi2             = trials&1;
                            ((DCI2A_20MHz_2A_FDD_t *)&DLSCH_alloc_pdu_1[0])->rv2              = round&3;
                          }

                          memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu_1[0],sizeof(DCI2A_20MHz_2A_FDD_t));
                          break;
                      }

                      break;

                    case 5:
                      DLSCH_alloc_pdu2_1E[0].ndi             = trials&1;
                      DLSCH_alloc_pdu2_1E[0].rv              = round&3;
                      memcpy(&dci_alloc[0].dci_pdu[0],&DLSCH_alloc_pdu2_1E[0],sizeof(DCI1E_5MHz_2A_M10PRB_TDD_t));
                      break;
                  }
                }
              }
            }

            num_pdcch_symbols_2 = generate_dci_top(num_ue_spec_dci + num_common_dci,
                                                   dci_alloc,
                                                   0,
                                                   AMP,
                                                   &eNB->frame_parms,
                                                   eNB->common_vars.txdataF[eNB_id],
                                                   subframe);

            if (num_pdcch_symbols_2 > num_pdcch_symbols) {
              msg("Error: given num_pdcch_symbols not big enough (%d > %d)\n",num_pdcch_symbols_2,num_pdcch_symbols);
              exit(-1);
            }

            for (k=0; k<n_users; k++) {
              for (int cw=0; cw<Kmimo; cw++) {
                coded_bits_per_codeword = get_G(&eNB->frame_parms,
                                                eNB->dlsch[k][cw]->harq_processes[0]->nb_rb,
                                                eNB->dlsch[k][cw]->harq_processes[0]->rb_alloc,
                                                get_Qm(eNB->dlsch[k][cw]->harq_processes[0]->mcs),
                                                eNB->dlsch[k][cw]->harq_processes[0]->Nl,
                                                num_pdcch_symbols,
                                                0,
                                                subframe,
                                                (transmission_mode<7?0:transmission_mode));
#ifdef TBS_FIX   // This is for MESH operation!!!
                tbs = (double)3*TBStable[get_I_TBS(eNB->dlsch[k][cw]->harq_processes[0]->mcs)][eNB->dlsch[k][cw]->nb_rb-1]/4;
#else
                tbs = eNB->dlsch[k][cw]->harq_processes[0]->TBS;
#endif
                rate = (double)tbs/(double)coded_bits_per_codeword;

                if ((SNR==snr0) && (trials==0) && (round==0))
                  printf("User %d, cw %d: Rate = %f (%f bits/dim) (G %u, TBS %u, mod %d, pdcch_sym %d, ndi %d)\n",
                         k,cw,rate,rate*get_Qm(eNB->dlsch[k][0]->harq_processes[0]->mcs),
                         coded_bits_per_codeword,
                         tbs,
                         get_Qm(eNB->dlsch[k][0]->harq_processes[0]->mcs),
                         num_pdcch_symbols,
                         eNB->dlsch[0][0]->harq_processes[0]->round);

                // use the PMI from previous trial
                if (DLSCH_alloc_pdu2_1E[0].tpmi == 5) {
                  eNB->dlsch[0][0]->harq_processes[0]->pmi_alloc = quantize_subband_pmi(&UE->measurements,0,eNB->frame_parms.N_RB_DL);
                  UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[0]->pmi_alloc = quantize_subband_pmi(&UE->measurements,0,UE->frame_parms.N_RB_DL);

                  if (n_users>1)
                    eNB->dlsch[1][0]->harq_processes[0]->pmi_alloc = (eNB->dlsch[0][0]->harq_processes[0]->pmi_alloc ^ 0x1555);

                  /*
                    if ((trials<10) && (round==0)) {
                    printf("tx PMI UE0 %x (pmi_feedback %d)\n",pmi2hex_2Ar1(eNB->dlsch[0][0]->pmi_alloc),pmi_feedback);
                    if (transmission_mode ==5)
                    printf("tx PMI UE1 %x\n",pmi2hex_2Ar1(eNB->dlsch[1][0]->pmi_alloc));
                    }
                  */
                }

                start_meas(&eNB->dlsch_encoding_stats);

                if (dlsch_encoding(eNB,
                                   ((cw==0) ? input_buffer0[k] : input_buffer1[k]),
                                   num_pdcch_symbols,
                                   eNB->dlsch[k][cw],
                                   0,subframe,
                                   &eNB->dlsch_rate_matching_stats,
                                   &eNB->dlsch_turbo_encoding_stats,
                                   &eNB->dlsch_interleaving_stats)<0)
                  exit(-1);

                /*
                if (transmission_mode == 3) {
                if (dlsch_encoding(input_buffer1[k],
                   &eNB->frame_parms,
                   num_pdcch_symbols,
                   eNB->dlsch[k][1],
                   0,subframe,
                   &eNB->dlsch_rate_matching_stats,
                   &eNB->dlsch_turbo_encoding_stats,
                   &eNB->dlsch_interleaving_stats
                   )<0)
                exit(-1);
                }
                */
                stop_meas(&eNB->dlsch_encoding_stats);
                eNB->dlsch[k][cw]->rnti = (common_flag==0) ? n_rnti+k : SI_RNTI;
                start_meas(&eNB->dlsch_scrambling_stats);
                dlsch_scrambling(&eNB->frame_parms,
                                 0,
                                 eNB->dlsch[k][cw],
                                 coded_bits_per_codeword,
                                 0,
                                 subframe<<1);
                stop_meas(&eNB->dlsch_scrambling_stats);

                if (n_frames==1) {
                  for (s=0; s<eNB->dlsch[k][cw]->harq_processes[0]->C; s++) {
                    if (s<eNB->dlsch[k][cw]->harq_processes[0]->Cminus)
                      Kr = eNB->dlsch[k][cw]->harq_processes[0]->Kminus;
                    else
                      Kr = eNB->dlsch[k][cw]->harq_processes[0]->Kplus;

                    Kr_bytes = Kr>>3;

                    for (i=0; i<Kr_bytes; i++)
                      printf("%d : (%x)\n",i,eNB->dlsch[k][cw]->harq_processes[0]->c[s][i]);
                  }
                }
              }

              start_meas(&eNB->dlsch_modulation_stats);
              re_allocated = dlsch_modulation(eNB,
                                              eNB->common_vars.txdataF[eNB_id],
                                              AMP,
                                              frame,
                                              subframe,
                                              num_pdcch_symbols,
                                              eNB->dlsch[k][0],
                                              eNB->dlsch[k][1]);
              /* avoid gcc warnings */
              (void)re_allocated;
              stop_meas(&eNB->dlsch_modulation_stats);
              /*
              if (trials==0 && round==0)
              printf("RE count %d\n",re_allocated);
              */
            } //n_users

            generate_pilots(eNB,
                            eNB->common_vars.txdataF[eNB_id],
                            AMP,
                            LTE_NUMBER_OF_SUBFRAMES_PER_FRAME);
            /*
                  //PSS/SSS
                        if (eNB->frame_parms.frame_type == FDD) {
                          generate_pss(eNB->common_vars.txdataF[0],
                                       AMP,
                                       &eNB->frame_parms,
                                       (eNB->frame_parms.Ncp==NORMAL) ? 6 : 5,
                                       0);
                          generate_sss(eNB->common_vars.txdataF[0],
                                       AMP,
                                       &eNB->frame_parms,
                                       (eNB->frame_parms.Ncp==NORMAL) ? 5 : 4,
                                       0);
                          generate_pss(eNB->common_vars.txdataF[0],
                                       AMP,
                                       &eNB->frame_parms,
                                       (eNB->frame_parms.Ncp==NORMAL) ? 6 : 5,
                                       10);
                          generate_sss(eNB->common_vars.txdataF[0],
                                       AMP,
                                       &eNB->frame_parms,
                                       (eNB->frame_parms.Ncp==NORMAL) ? 5 : 4,
                                       10);
                  }
                        else if (eNB->frame_parms.frame_type == TDD) {
                          generate_sss(eNB->common_vars.txdataF[0],
                                       AMP,
                                       &eNB->frame_parms,
                                       (eNB->frame_parms.Ncp==NORMAL) ? 6 : 5,
                                       1);
                          generate_pss(eNB->common_vars.txdataF[0],
                                       AMP,
                                       &eNB->frame_parms,
                                       2,
                                       2);
                          generate_sss(eNB->common_vars.txdataF[0],
                                       AMP,
                                       &eNB->frame_parms,
                                       (eNB->frame_parms.Ncp==NORMAL) ? 6 : 5,
                                       11);
                          generate_pss(eNB->common_vars.txdataF[0],
                                       AMP,
                                       &eNB->frame_parms,
                                       2,
                                       12);
                        }

                          //PBCH
                          pbch_pdu[2] = 0;

                          // FIXME setting pbch_pdu[2] to zero makes the switch statement easier: remove all the or-operators
                          switch (eNB->frame_parms.N_RB_DL) {
                          case 6:
                            pbch_pdu[2] = (pbch_pdu[2]&0x1f) | (0<<5);
                            break;

                          case 15:
                            pbch_pdu[2] = (pbch_pdu[2]&0x1f) | (1<<5);
                            break;

                          case 25:
                            pbch_pdu[2] = (pbch_pdu[2]&0x1f) | (2<<5);
                            break;

                          case 50:
                            pbch_pdu[2] = (pbch_pdu[2]&0x1f) | (3<<5);
                            break;

                          case 75:
                            pbch_pdu[2] = (pbch_pdu[2]&0x1f) | (4<<5);
                            break;

                          case 100:
                            pbch_pdu[2] = (pbch_pdu[2]&0x1f) | (5<<5);
                            break;

                          default:
                            // FIXME if we get here, this should be flagged as an error, right?
                            pbch_pdu[2] = (pbch_pdu[2]&0x1f) | (2<<5);
                            break;
                          }

                          pbch_pdu[2] = (pbch_pdu[2]&0xef) |
                                        ((eNB->frame_parms.phich_config_common.phich_duration << 4)&0x10);

                          switch (eNB->frame_parms.phich_config_common.phich_resource) {
                          case oneSixth:
                            pbch_pdu[2] = (pbch_pdu[2]&0xf3) | (0<<2);
                            break;

                          case half:
                            pbch_pdu[2] = (pbch_pdu[2]&0xf3) | (1<<2);
                            break;

                          case one:
                            pbch_pdu[2] = (pbch_pdu[2]&0xf3) | (2<<2);
                            break;

                          case two:
                            pbch_pdu[2] = (pbch_pdu[2]&0xf3) | (3<<2);
                            break;

                          default:
                            // unreachable
                            break;


                          pbch_pdu[2] = (pbch_pdu[2]&0xfc) | ((0>>8)&0x3);
                          pbch_pdu[1] = 0&0xfc;
                          pbch_pdu[0] = 0;
                        }

                        generate_pbch(&eNB->lte_eNB_pbch,
                                      eNB->common_vars.txdataF[0],
                                      AMP,
                                      &eNB->frame_parms,
                                      pbch_pdu,
                                      0&3);
            */
            start_meas(&eNB->ofdm_mod_stats);
            /*
                  for(i=0;i<20;i++){

                          do_OFDM_mod_l(eNB->common_vars.txdataF[eNB_id],
                                        eNB->common_vars.txdata[eNB_id],
                                        i,
                                        &eNB->frame_parms);
                    }

            */
            do_OFDM_mod_symbol(&eNB->common_vars,
                               eNB_id,
                               (subframe*2),
                               &eNB->frame_parms);
            do_OFDM_mod_symbol(&eNB->common_vars,
                               eNB_id,
                               (subframe*2)+1,
                               &eNB->frame_parms);
            stop_meas(&eNB->ofdm_mod_stats);
            stop_meas(&eNB->phy_proc_tx);

            /* do_OFDM_mod_l(&eNB->common_vars,
                           eNB_id,
                           (subframe*2)+2,
                           &eNB->frame_parms); */

            if (n_frames==1) {
              if (transmission_mode<7)
                LOG_M("txsigF0.m","txsF0", &eNB->common_vars.txdataF[eNB_id][0][subframe*nsymb*eNB->frame_parms.ofdm_symbol_size],
                      nsymb*eNB->frame_parms.ofdm_symbol_size,1,1);
              else if (transmission_mode==7)
                LOG_M("txsigF0.m","txsF0", &eNB->common_vars.txdataF[eNB_id][5][subframe*nsymb*eNB->frame_parms.ofdm_symbol_size],
                      nsymb*eNB->frame_parms.ofdm_symbol_size,1,1);

              LOG_M("txsigF0_BF.m","txsF0_BF", &eNB->common_vars.txdataF_BF[eNB_id][0][0],
                    eNB->frame_parms.ofdm_symbol_size,1,1);

              if (eNB->frame_parms.nb_antennas_tx>1)// to be updated
                LOG_M("txsigF1.m","txsF1", &eNB->common_vars.txdataF[eNB_id][1][subframe*nsymb*eNB->frame_parms.ofdm_symbol_size],
                      nsymb*eNB->frame_parms.ofdm_symbol_size,1,1);
            }

            tx_lev = 0;

            for (aa=0; aa<eNB->frame_parms.nb_antennas_tx; aa++) {
              tx_lev += signal_energy(&eNB->common_vars.txdata[eNB_id][aa]
                                      [subframe*eNB->frame_parms.samples_per_tti],
                                      eNB->frame_parms.samples_per_tti);
            }

            tx_lev_dB = (unsigned int) dB_fixed(tx_lev);

            if (n_frames==1) {
              printf("tx_lev = %u (%u dB)\n",tx_lev,tx_lev_dB);
              LOG_M("txsig0.m","txs0", &eNB->common_vars.txdata[eNB_id][0][subframe*eNB->frame_parms.samples_per_tti],eNB->frame_parms.samples_per_tti,1,1);
              // LOG_M("txsig0.m","txs0",&eNB->common_vars.txdata[eNB_id][0][0*eNB->frame_parms.samples_per_tti],eNB->frame_parms.samples_per_tti*10,1,1);
            }
          }

          /*
            else {  // Read signal from file
            i=0;
            while (!feof(input_fd)) {
            fscanf(input_fd,"%s %s",input_val_str,input_val_str2);

            if ((i%4)==0) {
            ((short*)txdata[0])[i/2] = (short)((1<<15)*strtod(input_val_str,NULL));
            ((short*)txdata[0])[(i/2)+1] = (short)((1<<15)*strtod(input_val_str2,NULL));
            if ((i/4)<100)
            printf("sample %d => %e + j%e (%d +j%d)\n",i/4,strtod(input_val_str,NULL),strtod(input_val_str2,NULL),((short*)txdata[0])[i/4],((short*)txdata[0])[(i/4)+1]);//1,input_val2,);
            }
            i++;
            if (i>(FRAME_LENGTH_SAMPLES))
            break;
            }
            printf("Read in %d samples\n",i/4);
            LOG_M("txsig0.m","txs0", txdata[0],2*frame_parms->samples_per_tti,1,1);
            //    LOG_M("txsig1.m","txs1", txdata[1],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);
            tx_lev = signal_energy(&txdata[0][0],
            OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES);
            tx_lev_dB = (unsigned int) dB_fixed(tx_lev);
            }
          */

          //    printf("Copying tx ..., nsymb %d (n_tx %d), awgn %d\n",nsymb,eNB->frame_parms.nb_antennas_tx,awgn_flag);
          for (i=0; i<2*frame_parms->samples_per_tti; i++) {
            for (aa=0; aa<eNB->frame_parms.nb_antennas_tx; aa++) {
              if (awgn_flag == 0) {
                s_re[aa][i] = ((double)(((short *)eNB->common_vars.txdata[eNB_id][aa]))[(2*subframe*UE->frame_parms.samples_per_tti) + (i<<1)]);
                s_im[aa][i] = ((double)(((short *)eNB->common_vars.txdata[eNB_id][aa]))[(2*subframe*UE->frame_parms.samples_per_tti) +(i<<1)+1]);
              } else {
                for (aarx=0; aarx<UE->frame_parms.nb_antennas_rx; aarx++) {
                  if (aa==0) {
                    r_re[aarx][i] = (double)(((short *)eNB->common_vars.txdata[eNB_id][aa]))[(2*subframe*UE->frame_parms.samples_per_tti) +(i<<1)];
                    r_im[aarx][i] = (double)(((short *)eNB->common_vars.txdata[eNB_id][aa]))[(2*subframe*UE->frame_parms.samples_per_tti) +(i<<1)+1];
                    //printf("r[%d][%d]=> %f, %f\n",aarx,i,r_re[aarx][i],r_im[aarx][i]);
                  } else {
                    r_re[aarx][i] += (double)(((short *)eNB->common_vars.txdata[eNB_id][aa]))[(2*subframe*UE->frame_parms.samples_per_tti) +(i<<1)];
                    r_im[aarx][i] += (double)(((short *)eNB->common_vars.txdata[eNB_id][aa]))[(2*subframe*UE->frame_parms.samples_per_tti) +(i<<1)+1];
                  }
                }
              }
            }
          }

          // Multipath channel
          if (awgn_flag == 0) {
            multipath_channel(eNB2UE[round],s_re,s_im,r_re,r_im,
                              2*frame_parms->samples_per_tti,hold_channel);

            //      printf("amc: ****************** eNB2UE[%d]->n_rx = %d,dd %d\n",round,eNB2UE[round]->nb_rx,eNB2UE[round]->channel_offset);
            if(abstx==1 && num_rounds>1)
              if(round==0 && hold_channel==0) {
                random_channel(eNB2UE[1],0);
                random_channel(eNB2UE[2],0);
                random_channel(eNB2UE[3],0);
              }

            if (UE->perfect_ce==1) {
              // fill in perfect channel estimates
              freq_channel(eNB2UE[round],UE->frame_parms.N_RB_DL,12*UE->frame_parms.N_RB_DL + 1);
              /*
              LOG_M("channel.m","ch",eNB2UE[round]->ch[0],eNB2UE[round]->channel_length,1,8);
                          LOG_M("channelF.m","chF",eNB2UE[round]->chF[0],12*UE->frame_parms.N_RB_DL + 1,1,8);
              */
            }
          }

          if(abstx) {
            if (trials==0 && round==0) {
              // calculate freq domain representation to compute SINR
              freq_channel(eNB2UE[0], NB_RB,2*NB_RB + 1);
              // snr=pow(10.0,.1*SNR);
              fprintf(csv_fd,"%f,",SNR);

              for (u=0; u<2*NB_RB; u++) {
                for (aarx=0; aarx<eNB2UE[0]->nb_rx; aarx++) {
                  for (aatx=0; aatx<eNB2UE[0]->nb_tx; aatx++) {
                    channelx = eNB2UE[0]->chF[aarx+(aatx*eNB2UE[0]->nb_rx)][u].x;
                    channely = eNB2UE[0]->chF[aarx+(aatx*eNB2UE[0]->nb_rx)][u].y;
                    fprintf(csv_fd,"%e+i*(%e),",channelx,channely);
                  }
                }
              }

              if(num_rounds>1) {
                freq_channel(eNB2UE[1], NB_RB,2*NB_RB + 1);

                for (u=0; u<2*NB_RB; u++) {
                  for (aarx=0; aarx<eNB2UE[1]->nb_rx; aarx++) {
                    for (aatx=0; aatx<eNB2UE[1]->nb_tx; aatx++) {
                      channelx = eNB2UE[1]->chF[aarx+(aatx*eNB2UE[1]->nb_rx)][u].x;
                      channely = eNB2UE[1]->chF[aarx+(aatx*eNB2UE[1]->nb_rx)][u].y;
                      fprintf(csv_fd,"%e+i*(%e),",channelx,channely);
                    }
                  }
                }

                freq_channel(eNB2UE[2], NB_RB,2*NB_RB + 1);

                for (u=0; u<2*NB_RB; u++) {
                  for (aarx=0; aarx<eNB2UE[2]->nb_rx; aarx++) {
                    for (aatx=0; aatx<eNB2UE[2]->nb_tx; aatx++) {
                      channelx = eNB2UE[2]->chF[aarx+(aatx*eNB2UE[2]->nb_rx)][u].x;
                      channely = eNB2UE[2]->chF[aarx+(aatx*eNB2UE[2]->nb_rx)][u].y;
                      fprintf(csv_fd,"%e+i*(%e),",channelx,channely);
                    }
                  }
                }

                freq_channel(eNB2UE[3], NB_RB,2*NB_RB + 1);

                for (u=0; u<2*NB_RB; u++) {
                  for (aarx=0; aarx<eNB2UE[3]->nb_rx; aarx++) {
                    for (aatx=0; aatx<eNB2UE[3]->nb_tx; aatx++) {
                      channelx = eNB2UE[3]->chF[aarx+(aatx*eNB2UE[3]->nb_rx)][u].x;
                      channely = eNB2UE[3]->chF[aarx+(aatx*eNB2UE[3]->nb_rx)][u].y;
                      fprintf(csv_fd,"%e+i*(%e),",channelx,channely);
                    }
                  }
                }
              }
            }
          }

          //AWGN
          // This is the SNR on the PDSCH for OFDM symbols without pilots -> rho_A
          sigma2_dB = 10*log10((double)tx_lev) +10*log10((double)eNB->frame_parms.ofdm_symbol_size/(double)(NB_RB*12)) - SNR - get_pa_dB(eNB->pdsch_config_dedicated);
          sigma2 = pow(10,sigma2_dB/10);

          if (n_frames==1)
            printf("Sigma2 %f (sigma2_dB %f,%f,%f )\n",sigma2,sigma2_dB,10*log10((double)eNB->frame_parms.ofdm_symbol_size/(double)(NB_RB*12)),get_pa_dB(eNB->pdsch_config_dedicated));

          for (i=0; i<2*frame_parms->samples_per_tti; i++) {
            for (aa=0; aa<eNB->frame_parms.nb_antennas_rx; aa++) {
              // printf("s_re[0][%d]=> %f , r_re[0][%d]=> %f\n",i,s_re[aa][i],i,r_re[aa][i]);
              ((short *) UE->common_vars.rxdata[aa])[(2*subframe*UE->frame_parms.samples_per_tti)+2*i] =
                (short) (r_re[aa][i] + sqrt(sigma2/2)*gaussdouble(0.0,1.0));
              ((short *) UE->common_vars.rxdata[aa])[(2*subframe*UE->frame_parms.samples_per_tti)+2*i+1] =
                (short) (r_im[aa][i] + (iqim*r_re[aa][i]) + sqrt(sigma2/2)*gaussdouble(0.0,1.0));
              //((short*) UE->common_vars.rxdata[aa])[(2*subframe*UE->frame_parms.samples_per_tti)+2*i] = (short) r_re[aa][i];
              //((short*) UE->common_vars.rxdata[aa])[(2*subframe*UE->frame_parms.samples_per_tti)+2*i+1] = (short) r_im[aa][i];
              //printf("rxdata[%d][%d]=> %d, %d\n",aa,subframe*UE->frame_parms.samples_per_tti+i,r_re[aa][i],r_im[aa][i]);
            }
          }

          //    lte_sync_time_init(eNB->frame_parms,common_vars);
          //    lte_sync_time(common_vars->rxdata, eNB->frame_parms);
          //    lte_sync_time_free();

          /*
            // optional: read rx_frame from file
            if ((rx_frame_file = fopen("rx_frame.dat","r")) == NULL)
            {
            printf("Cannot open rx_frame.m data file\n");
            exit(0);
            }

            result = fread((void *)PHY_vars->rx_vars[0].RX_DMA_BUFFER,4,FRAME_LENGTH_COMPLEX_SAMPLES,rx_frame_file);
            printf("Read %d bytes\n",result);
            result = fread((void *)PHY_vars->rx_vars[1].RX_DMA_BUFFER,4,FRAME_LENGTH_COMPLEX_SAMPLES,rx_frame_file);
            printf("Read %d bytes\n",result);

            fclose(rx_frame_file);
          */

          if (n_frames==1) {
            printf("RX level in null symbol %d\n",dB_fixed(signal_energy(&UE->common_vars.rxdata[0][160+OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES],OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES/2)));
            printf("RX level in data symbol %d\n",dB_fixed(signal_energy(&UE->common_vars.rxdata[0][160+(2*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES)],OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES/2)));
            printf("rx_level Null symbol %f\n",10*log10(signal_energy_fp(r_re,r_im,1,OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES/2,256+(OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES))));
            printf("rx_level data symbol %f\n",10*log10(signal_energy_fp(r_re,r_im,1,OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES/2,256+(2*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES))));
          }

          if (eNB->frame_parms.Ncp == 0) {  // normal prefix
            pilot1 = 4;
            pilot2 = 7;
            pilot3 = 11;
          } else { // extended prefix
            pilot1 = 3;
            pilot2 = 6;
            pilot3 = 9;
          }

          start_meas(&UE->phy_proc_rx[UE->current_thread_id[subframe]]);

          // Inner receiver scheduling for 3 slots

          for (Ns=(2*subframe); Ns<((2*subframe)+3); Ns++) {
            for (l=0; l<pilot2; l++) {
              if (n_frames==1)
                printf("Ns %d, l %d, l2 %d\n",Ns, l, l+(Ns%2)*pilot2);

              /*
              This function implements the OFDM front end processor (FEP).

              Parameters:
              frame_parms  LTE DL Frame Parameters
              ue_common_vars   LTE UE Common Vars
              l  symbol within slot (0..6/7)
              Ns   Slot number (0..19)
              sample_offset  offset within rxdata (points to beginning of subframe)
              no_prefix  if 1 prefix is removed by HW

              */
              start_meas(&UE->ofdm_demod_stats);
              slot_fep(UE,
                       l,
                       Ns%20,
                       0,
                       0,
                       0);
              stop_meas(&UE->ofdm_demod_stats);

              if (UE->perfect_ce==1) {
                if (awgn_flag==0) {
                  for(k=0; k<NUMBER_OF_eNB_MAX; k++) {
                    for(aa=0; aa<frame_parms->nb_antenna_ports_eNB; aa++) {
                      for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
                        for (i=0; i<frame_parms->N_RB_DL*12; i++) {
                          ((int16_t *) UE->common_vars.common_vars_rx_data_per_thread[UE->current_thread_id[subframe]].dl_ch_estimates[k][(aa<<1)+aarx])[2*i+((l+(Ns%2)*pilot2)*frame_parms->ofdm_symbol_size
                              +LTE_CE_FILTER_LENGTH)*2]=(int16_t)(
                                                          eNB2UE[round]->chF[aarx+(aa*frame_parms->nb_antennas_rx)][i].x*AMP);
                          //printf("x=%d,AMP=%d\n",eNB2UE[round]->chF[aarx+(aa*frame_parms->nb_antennas_rx)][i].x,AMP);
                          ((int16_t *) UE->common_vars.common_vars_rx_data_per_thread[UE->current_thread_id[subframe]].dl_ch_estimates[k][(aa<<1)+aarx])[2*i+1+((l+(Ns%2)*pilot2)*frame_parms->ofdm_symbol_size
                              +LTE_CE_FILTER_LENGTH)*2]=(int16_t)(
                                                          eNB2UE[round]->chF[aarx+(aa*frame_parms->nb_antennas_rx)][i].y*AMP);

                          if (transmission_mode == 7) {
                            //this should include the BF weights! Will not work for a random channel
                            if (UE->high_speed_flag==0) {
                              ((int16_t *)UE->pdsch_vars[UE->current_thread_id[subframe]][0]->dl_bf_ch_estimates[(aa<<1)+aarx])[2*i]=(int16_t)(
                                    eNB2UE[round]->chF[aarx+(aa*frame_parms->nb_antennas_rx)][i].x*AMP);
                              ((int16_t *)UE->pdsch_vars[UE->current_thread_id[subframe]][0]->dl_bf_ch_estimates[(aa<<1)+aarx])[2*i+1]=(int16_t)(
                                    eNB2UE[round]->chF[aarx+(aa*frame_parms->nb_antennas_rx)][i].y*AMP);
                              //printf("**,x=%d,AMP=%d\n",eNB2UE[round]->chF[aarx+(aa*frame_parms->nb_antennas_rx)][i].x,AMP);
                            } else  {
                              ((int16_t *)UE->pdsch_vars[UE->current_thread_id[subframe]][0]->dl_bf_ch_estimates[(aa<<1)+aarx])[2*i+((l+(Ns%2)*pilot2)*frame_parms->ofdm_symbol_size)*2]=(int16_t)(
                                    eNB2UE[round]->chF[aarx+(aa*frame_parms->nb_antennas_rx)][i].x*AMP);
                              ((int16_t *)UE->pdsch_vars[UE->current_thread_id[subframe]][0]->dl_bf_ch_estimates[(aa<<1)+aarx])[2*i+1+((l+(Ns%2)*pilot2)*frame_parms->ofdm_symbol_size)*2]=(int16_t)(
                                    eNB2UE[round]->chF[aarx+(aa*frame_parms->nb_antennas_rx)][i].y*AMP);
                            }
                          }
                        }
                      }
                    }
                  }
                } else {
                  for(aa=0; aa<frame_parms->nb_antenna_ports_eNB; aa++) {
                    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
                      for (i=0; i<frame_parms->N_RB_DL*12; i++) {
                        ((int16_t *) UE->common_vars.common_vars_rx_data_per_thread[UE->current_thread_id[subframe]].dl_ch_estimates[0][(aa<<1)+aarx])[2*i+((l+(Ns%2)*pilot2)*frame_parms->ofdm_symbol_size
                            +LTE_CE_FILTER_LENGTH)*2]=(short)(AMP);
                        ((int16_t *) UE->common_vars.common_vars_rx_data_per_thread[UE->current_thread_id[subframe]].dl_ch_estimates[0][(aa<<1)+aarx])[2*i+1+((l+(Ns%2)*pilot2)*frame_parms->ofdm_symbol_size
                            +LTE_CE_FILTER_LENGTH)*2]=0/2;

                        if (transmission_mode == 7) {
                          if (UE->high_speed_flag==0) {
                            ((int16_t *) UE->pdsch_vars[UE->current_thread_id[subframe]][0]->dl_bf_ch_estimates[(aa<<1)+aarx])[2*i]=(short)(AMP);
                            ((int16_t *) UE->pdsch_vars[UE->current_thread_id[subframe]][0]->dl_bf_ch_estimates[(aa<<1)+aarx])[2*i+1]=0/2;
                          } else {
                            ((int16_t *) UE->pdsch_vars[UE->current_thread_id[subframe]][0]->dl_bf_ch_estimates[(aa<<1)+aarx])[2*i+((l+(Ns%2)*pilot2)*frame_parms->ofdm_symbol_size)*2]=(short)(AMP);
                            ((int16_t *) UE->pdsch_vars[UE->current_thread_id[subframe]][0]->dl_bf_ch_estimates[(aa<<1)+aarx])[2*i+1+((l+(Ns%2)*pilot2)*frame_parms->ofdm_symbol_size)*2]=0/2;
                          }
                        }
                      }
                    }
                  }
                }
              }

              if ((Ns==((2*subframe))) && (l==0)) {
                lte_ue_measurements(UE,
                                    subframe*UE->frame_parms.samples_per_tti,
                                    1,
                                    0);
                /*
                  debug_msg("RX RSSI %d dBm, digital (%d, %d) dB, linear (%d, %d), avg rx power %d dB (%d lin), RX gain %d dB\n",
                  UE->measurements.rx_rssi_dBm[0] - ((UE->frame_parms.nb_antennas_rx==2) ? 3 : 0),
                  UE->measurements.wideband_cqi_dB[0][0],
                  UE->measurements.wideband_cqi_dB[0][1],
                  UE->measurements.wideband_cqi[0][0],
                  UE->measurements.wideband_cqi[0][1],
                  UE->measurements.rx_power_avg_dB[0],
                  UE->measurements.rx_power_avg[0],
                  UE->rx_total_gain_dB);
                  debug_msg("N0 %d dBm digital (%d, %d) dB, linear (%d, %d), avg noise power %d dB (%d lin)\n",
                  UE->measurements.n0_power_tot_dBm,
                  UE->measurements.n0_power_dB[0],
                  UE->measurements.n0_power_dB[1],
                  UE->measurements.n0_power[0],
                  UE->measurements.n0_power[1],
                  UE->measurements.n0_power_avg_dB,
                  UE->measurements.n0_power_avg);
                  debug_msg("Wideband CQI tot %d dB, wideband cqi avg %d dB\n",
                  UE->measurements.wideband_cqi_tot[0],
                  UE->measurements.wideband_cqi_avg[0]);
                */

                if (transmission_mode==5 || transmission_mode==6) {
                  if (pmi_feedback == 1) {
                    pmi_feedback = 0;
                    hold_channel = 1;
                    goto PMI_FEEDBACK;
                  }
                }
              }

              if ((Ns==(2*subframe)) && (l==pilot1)) {// process symbols 0,1,2
                if (dci_flag == 1) {
                  UE->UE_mode[0] = PUSCH;
                  start_meas(&UE->dlsch_rx_pdcch_stats);
                  rx_pdcch(&UE->common_vars,
                           UE->pdcch_vars,
                           &UE->frame_parms,
                           0, // frame
                           subframe,
                           0,
                           (UE->frame_parms.mode1_flag == 1) ? SISO : ALAMOUTI,
                           UE->high_speed_flag,
                           0);
                  stop_meas(&UE->dlsch_rx_pdcch_stats);
                  // overwrite number of pdcch symbols
                  UE->pdcch_vars[0]->num_pdcch_symbols = num_pdcch_symbols;
                  dci_cnt = dci_decoding_procedure(UE,
                                                   dci_alloc_rx,1,
                                                   eNB_id,
                                                   subframe);
                  //printf("dci_cnt %d\n",dci_cnt);

                  if (dci_cnt==0) {
                    dlsch_active = 0;

                    if (round==0) {
                      dci_errors++;
                      round=5;
                      errs[0]++;
                      //round_trials[0]++;

                      if (n_frames==1)
                        printf("DCI error trial %u errs[0] %u\n",trials,errs[0]);
                    }

                    //    for (i=1;i<=round;i++)
                    //      round_trials[i]--;
                    //    round=5;
                  }

                  for (i=0; i<dci_cnt; i++) {
                    //        printf("Generating dlsch parameters for RNTI %x\n",dci_alloc_rx[i].rnti);
                    if (round == 0) UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[UE->dlsch[UE->current_thread_id[subframe]][0][0]->current_harq_pid]->first_tx=1;

                    if ((dci_alloc_rx[i].rnti == n_rnti) &&
                        (generate_ue_dlsch_params_from_dci(0,
                                                           subframe,
                                                           dci_alloc_rx[i].dci_pdu,
                                                           dci_alloc_rx[i].rnti,
                                                           dci_alloc_rx[i].format,
                                                           UE->pdcch_vars[UE->current_thread_id[subframe]][eNB_id],
                                                           UE->pdsch_vars[UE->current_thread_id[subframe]][eNB_id],
                                                           UE->dlsch[UE->current_thread_id[subframe]][0],
                                                           &UE->frame_parms,
                                                           UE->pdsch_config_dedicated,
                                                           SI_RNTI,
                                                           0,
                                                           P_RNTI==0,
                                                           transmission_mode<7?0:transmission_mode))) {
                      //dump_dci(&UE->frame_parms,&dci_alloc_rx[i]);
                      coded_bits_per_codeword = get_G(&eNB->frame_parms,
                                                      UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[UE->dlsch[UE->current_thread_id[subframe]][0][0]->current_harq_pid]->nb_rb,
                                                      UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[UE->dlsch[UE->current_thread_id[subframe]][0][0]->current_harq_pid]->rb_alloc_even,
                                                      get_Qm(UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[UE->dlsch[UE->current_thread_id[subframe]][0][0]->current_harq_pid]->mcs),
                                                      UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[UE->dlsch[UE->current_thread_id[subframe]][0][0]->current_harq_pid]->Nl,
                                                      UE->pdcch_vars[0]->num_pdcch_symbols,
                                                      0,
                                                      subframe,
                                                      (transmission_mode<7?0:transmission_mode));
                      /*if (transmission_mode==7 && common_flag==0)
                          UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[0]->mimo_mode = TM7; */
                      /*
                      rate = (double)dlsch_tbs25[get_I_TBS(UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[UE->dlsch[UE->current_thread_id[subframe]][0][0]->current_harq_pid]->mcs)][UE->dlsch[UE->current_thread_id[subframe]][0][0]->nb_rb-1]/(coded_bits_per_codeword);
                      rate*=get_Qm(UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[UE->dlsch[UE->current_thread_id[subframe]][0][0]->current_harq_pid]->mcs);
                      */
                      printf("num_pdcch_symbols %d, G %u, TBS %d\n",UE->pdcch_vars[0]->num_pdcch_symbols,coded_bits_per_codeword,
                             UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[UE->dlsch[UE->current_thread_id[subframe]][0][0]->current_harq_pid]->TBS);
                      dlsch_active = 1;
                    } else {
                      dlsch_active = 0;

                      if (round==0) {
                        dci_errors++;
                        errs[0]++;
                        //round_trials[0]++;
                        round=5;

                        if (n_frames==1)
                          printf("DCI misdetection trial %u\n",trials);

                      }

                      //      for (i=1;i<=round;i++)
                      //        round_trials[i]--;
                      //      round=5;
                    }
                  }
                }  // if dci_flag==1
                else { //dci_flag == 0
                  UE->pdcch_vars[0]->crnti = n_rnti;
                  UE->pdcch_vars[0]->num_pdcch_symbols = num_pdcch_symbols;

                  if (round == 0) UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[0]->first_tx=1;

                  switch (transmission_mode) {
                    case 1:
                    case 2:
                    case 7:
                      generate_ue_dlsch_params_from_dci(0,
                                                        subframe,
                                                        &DLSCH_alloc_pdu_1[0],
                                                        (common_flag==0)? C_RNTI : SI_RNTI,
                                                        (common_flag==0)? format1 : format1A,
                                                        UE->pdcch_vars[UE->current_thread_id[subframe]][eNB_id],
                                                        UE->pdsch_vars[UE->current_thread_id[subframe]][eNB_id],
                                                        UE->dlsch[UE->current_thread_id[subframe]][0],
                                                        &UE->frame_parms,
                                                        UE->pdsch_config_dedicated,
                                                        SI_RNTI,
                                                        0,
                                                        P_RNTI,
                                                        transmission_mode<7?0:transmission_mode);
                      /*if(transmission_mode==7 && common_flag==0)
                          UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[0]->mimo_mode = TM7;*/
                      break;

                    case 3:
                      //        printf("Rate: TM3 (before) round %d (%d) first_tx %d\n",round,UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[0]->round,UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[0]->first_tx);
                      generate_ue_dlsch_params_from_dci(0,
                                                        subframe,
                                                        &DLSCH_alloc_pdu_1[0],
                                                        (common_flag==0)? C_RNTI : SI_RNTI,
                                                        (common_flag==0)? format2A : format1A,
                                                        UE->dlsch[UE->current_thread_id[subframe]][0],
                                                        &UE->frame_parms,
                                                        UE->pdsch_config_dedicated,
                                                        SI_RNTI,
                                                        0,
                                                        P_RNTI,
                                                        0);
                      //        printf("Rate: TM3 (after) round %d (%d) first_tx %d\n",round,UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[0]->round,UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[0]->first_tx);
                      break;

                    case 4:
                      generate_ue_dlsch_params_from_dci(0,
                                                        subframe,
                                                        &DLSCH_alloc_pdu_1[0],
                                                        (common_flag==0)? C_RNTI : SI_RNTI,
                                                        (common_flag==0)? format2 : format1A,
                                                        UE->pdcch_vars[UE->current_thread_id[subframe]][eNB_id],
                                                        UE->pdsch_vars[UE->current_thread_id[subframe]][eNB_id],
                                                        UE->dlsch[UE->current_thread_id[subframe]][0],
                                                        &UE->frame_parms,
                                                        UE->pdsch_config_dedicated,
                                                        SI_RNTI,
                                                        0,
                                                        P_RNTI,
                                                        0);
                      break;

                    case 5:
                    case 6:
                      generate_ue_dlsch_params_from_dci(0,
                                                        subframe,
                                                        &DLSCH_alloc_pdu2_1E[0],
                                                        C_RNTI,
                                                        format1E_2A_M10PRB,
                                                        ue->pdcch_vars[UE->current_thread_id[subframe]][eNB_id],
                                                        ue->pdsch_vars[UE->current_thread_id[subframe]][eNB_id],
                                                        UE->dlsch[UE->current_thread_id[subframe]][0],
                                                        &UE->frame_parms,
                                                        UE->pdsch_config_dedicated,
                                                        SI_RNTI,
                                                        0,
                                                        P_RNTI,
                                                        0);
                      break;
                  }

                  dlsch_active = 1;
                } // if dci_flag == 1
              }

              if (dlsch_active == 1) {
                if (transmission_mode==7) {
                  if (UE->frame_parms.Ncp==0) {
                    if ((Ns==(2*subframe)   && ((l==3) || (l==6))) ||
                        (Ns==(1+2*subframe) && ((l==2) || (l==5)))) {
                      if (perfect_ce==0)
                        lte_dl_bf_channel_estimation(UE,eNB_id,0,Ns,5,l+7*(Ns%2==1));
                    }
                  } else {
                    msg("Beamforming channel estimation not supported yet for TM7 extented CP.\n");
                  }
                }

                if ((Ns==(1+(2*subframe))) && (l==0)) {// process PDSCH symbols 1,2,3,4,5,(6 Normal Prefix)
                  if ((transmission_mode == 5) &&
                      (UE->dlsch[UE->current_thread_id[subframe]][eNB_id][0]->harq_processes[UE->dlsch[UE->current_thread_id[subframe]][0][0]->current_harq_pid]->dl_power_off==0) &&
                      (UE->use_ia_receiver ==1)) {
                    dual_stream_UE = 1;
                  } else {
                    dual_stream_UE = 0;
                  }

                  start_meas(&UE->dlsch_llr_stats);

                  for (m=UE->pdcch_vars[0]->num_pdcch_symbols;
                       m<pilot2;
                       m++) {
                    if (rx_pdsch(UE,
                                 PDSCH,
                                 eNB_id,
                                 eNB_id_i,
                                 0, // frame
                                 subframe,
                                 m,
                                 (m==UE->pdcch_vars[0]->num_pdcch_symbols)?1:0,
                                 dual_stream_UE,
                                 i_mod,
                                 UE->dlsch[UE->current_thread_id[subframe]][0][0]->current_harq_pid)==-1) {
                      dlsch_active = 0;
                      break;
                    }
                  }

                  stop_meas(&UE->dlsch_llr_stats);
                }

                if ((Ns==(1+(2*subframe))) && (l==pilot1)) {
                  // process symbols (6 Extended Prefix),7,8,9
                  start_meas(&UE->dlsch_llr_stats);

                  for (m=pilot2;
                       m<pilot3;
                       m++) {
                    if (rx_pdsch(UE,
                                 PDSCH,
                                 eNB_id,
                                 eNB_id_i,
                                 0, // frame
                                 subframe,
                                 m,
                                 0,
                                 dual_stream_UE,
                                 i_mod,
                                 UE->dlsch[UE->current_thread_id[subframe]][0][0]->current_harq_pid)==-1) {
                      dlsch_active=0;
                      break;
                    }
                  }

                  stop_meas(&UE->dlsch_llr_stats);
                }

                if ((Ns==(2+(2*subframe))) && (l==0)) { // process symbols 10,11,(12,13 Normal Prefix) do deinterleaving for TTI
                  start_meas(&UE->dlsch_llr_stats);

                  for (m=pilot3;
                       m<UE->frame_parms.symbols_per_tti;
                       m++) {
                    //printf("m=%d\n",m);
                    if (rx_pdsch(UE,
                                 PDSCH,
                                 eNB_id,
                                 eNB_id_i,
                                 0, // frame
                                 subframe,
                                 m,
                                 0,
                                 dual_stream_UE,
                                 i_mod,
                                 UE->dlsch[UE->current_thread_id[subframe]][0][0]->current_harq_pid)==-1) {
                      dlsch_active=0;
                      break;
                    }
                  }

                  stop_meas(&UE->dlsch_llr_stats);
                }

                if (test_perf ==0 ) {
                  if ((n_frames==1) && (Ns==(2+(2*subframe))) && (l==0))  {
                    LOG_M("ch0.m","ch0",eNB2UE[0]->ch[0],eNB2UE[0]->channel_length,1,8);

                    if (eNB->frame_parms.nb_antennas_tx>1)
                      LOG_M("ch1.m","ch1",eNB2UE[0]->ch[eNB->frame_parms.nb_antennas_rx],eNB2UE[0]->channel_length,1,8);

                    //common vars
                    //LOG_M("rxsig0.m","rxs0", &UE->common_vars.rxdata[0][0],10*UE->frame_parms.samples_per_tti,1,1);
                    LOG_M("rxsig0.m","rxs0", &UE->common_vars.rxdata[0][subframe*UE->frame_parms.samples_per_tti],UE->frame_parms.samples_per_tti,1,1);
                    //LOG_M("rxsigF0.m","rxsF0", &UE->common_vars.rxdataF[0][subframe*nsymb*eNB->frame_parms.ofdm_symbol_size],UE->frame_parms.ofdm_symbol_size*nsymb,1,1);
                    LOG_M("rxsigF0.m","rxsF0", &UE->common_vars.common_vars_rx_data_per_thread[UE->current_thread_id[subframe]].rxdataF[0][0],UE->frame_parms.ofdm_symbol_size*nsymb,1,1);

                    if (UE->frame_parms.nb_antennas_rx>1) {
                      LOG_M("rxsig1.m","rxs1", UE->common_vars.rxdata[1],UE->frame_parms.samples_per_tti,1,1);
                      LOG_M("rxsigF1.m","rxsF1", UE->common_vars..common_vars_rx_data_per_thread[UE->current_thread_id[subframe]]rxdataF[1],UE->frame_parms.ofdm_symbol_size*nsymb,1,1);
                    }

                    LOG_M("dlsch00_r0.m","dl00_r0",
                          &(UE->common_vars.common_vars_rx_data_per_thread[UE->current_thread_id[subframe]].dl_ch_estimates[eNB_id][0][0]),
                          UE->frame_parms.ofdm_symbol_size*nsymb,1,1);

                    if (UE->frame_parms.nb_antennas_rx>1)
                      LOG_M("dlsch01_r0.m","dl01_r0",
                            &(UE->common_vars.common_vars_rx_data_per_thread[UE->current_thread_id[subframe]].dl_ch_estimates[eNB_id][1][0]),
                            UE->frame_parms.ofdm_symbol_size*nsymb,1,1);

                    if (eNB->frame_parms.nb_antennas_tx>1)
                      LOG_M("dlsch10_r0.m","dl10_r0",
                            &(UE->common_vars.common_vars_rx_data_per_thread[UE->current_thread_id[subframe]].dl_ch_estimates[eNB_id][2][0]),
                            UE->frame_parms.ofdm_symbol_size*nsymb,1,1);

                    if ((UE->frame_parms.nb_antennas_rx>1) && (eNB->frame_parms.nb_antennas_tx>1))
                      LOG_M("dlsch11_r0.m","dl11_r0",
                            &(UE->common_vars.common_vars_rx_data_per_thread[UE->current_thread_id[subframe]].dl_ch_estimates[eNB_id][3][0]),
                            UE->frame_parms.ofdm_symbol_size*nsymb/2,1,1);

                    //pdsch_vars
                    dump_dlsch2(UE,eNB_id,subframe,&coded_bits_per_codeword,round);
                    //dump_dlsch2(UE,eNB_id_i,coded_bits_per_codeword);
                    LOG_M("dlsch_e.m","e",eNB->dlsch[0][0]->harq_processes[0]->e,coded_bits_per_codeword,1,4);
                    //pdcch_vars
                    LOG_M("pdcchF0_ext.m","pdcchF_ext", UE->pdcch_vars[eNB_id]->rxdataF_ext[0],2*3*UE->frame_parms.ofdm_symbol_size,1,1);
                    LOG_M("pdcch00_ch0_ext.m","pdcch00_ch0_ext",UE->pdcch_vars[eNB_id]->dl_ch_estimates_ext[0],300*3,1,1);
                    LOG_M("pdcch_rxF_comp0.m","pdcch0_rxF_comp0",UE->pdcch_vars[eNB_id]->rxdataF_comp[0],4*300,1,1);
                    LOG_M("pdcch_rxF_llr.m","pdcch_llr",UE->pdcch_vars[eNB_id]->llr,2400,1,4);
                  }
                }
              }
            }
          }

          //saving PMI in case of Transmission Mode > 5

          if(abstx) {
            if (trials==0 && round==0 && transmission_mode>=5 && transmission_mode<7) {
              for (iii=0; iii<NB_RB; iii++) {
                //fprintf(csv_fd, "%d, %d", (UE->pdsch_vars[eNB_id]->pmi_ext[iii]),(UE->pdsch_vars[eNB_id_i]->pmi_ext[iii]));
                fprintf(csv_fd,"%x,%x,",(UE->pdsch_vars[UE->current_thread_id[subframe]][eNB_id]->pmi_ext[iii]),(UE->pdsch_vars[eNB_id]->pmi_ext[iii]));
                printf("%x ",(UE->pdsch_vars[UE->current_thread_id[subframe]][eNB_id]->pmi_ext[iii]));
              }
            }
          }

          for (int cw=Kmimo-1; cw>=0; cw--) {
            UE->dlsch[UE->current_thread_id[subframe]][0][cw]->rnti = (common_flag==0) ? n_rnti: SI_RNTI;
            coded_bits_per_codeword = get_G(&eNB->frame_parms,
                                            eNB->dlsch[0][cw]->harq_processes[0]->nb_rb,
                                            eNB->dlsch[0][cw]->harq_processes[0]->rb_alloc,
                                            get_Qm(eNB->dlsch[0][cw]->harq_processes[0]->mcs),
                                            eNB->dlsch[0][cw]->harq_processes[0]->Nl,
                                            num_pdcch_symbols,
                                            0,
                                            subframe,
                                            (transmission_mode<7?0:transmission_mode));
            UE->dlsch[UE->current_thread_id[subframe]][0][cw]->harq_processes[UE->dlsch[UE->current_thread_id[subframe]][0][cw]->current_harq_pid]->G = coded_bits_per_codeword;
            // calculate uncoded BLER
            uncoded_ber=0;

            for (i=0; i<coded_bits_per_codeword; i++)
              if (eNB->dlsch[0][0]->harq_processes[0]->e[i] != (UE->pdsch_vars[UE->current_thread_id[subframe]][0]->llr[0][i]<0)) {
                uncoded_ber_bit[i] = 1;
                uncoded_ber++;
              } else
                uncoded_ber_bit[i] = 0;

            uncoded_ber/=coded_bits_per_codeword;
            avg_ber += uncoded_ber;

            if (n_frames==1)
              LOG_M("uncoded_ber_bit.m","uncoded_ber_bit",uncoded_ber_bit,coded_bits_per_codeword,1,0);

            start_meas(&UE->dlsch_unscrambling_stats);
            dlsch_unscrambling(&UE->frame_parms,
                               0,
                               UE->dlsch[UE->current_thread_id[subframe]][0][cw],
                               coded_bits_per_codeword,
                               UE->pdsch_vars[UE->current_thread_id[subframe]][eNB_id]->llr[cw],
                               0,
                               subframe<<1);
            stop_meas(&UE->dlsch_unscrambling_stats);
            start_meas(&UE->dlsch_decoding_stats);
            ret = dlsch_decoding(UE,
                                 UE->pdsch_vars[UE->current_thread_id[subframe]][eNB_id]->llr[cw],
                                 &UE->frame_parms,
                                 UE->dlsch[UE->current_thread_id[subframe]][0][cw],
                                 UE->dlsch[UE->current_thread_id[subframe]][0][cw]->harq_processes[UE->dlsch[UE->current_thread_id[subframe]][0][cw]->current_harq_pid],
                                 subframe,
                                 UE->dlsch[UE->current_thread_id[subframe]][0][cw]->current_harq_pid,
                                 1,llr8_flag);
            stop_meas(&UE->dlsch_decoding_stats);

            if (cw==1) {
              if (ret <= UE->dlsch[UE->current_thread_id[subframe]][0][cw]->max_turbo_iterations) {
              } else {
                errs2[round]++;
              }
            }
          }

          stop_meas(&UE->phy_proc_rx[UE->current_thread_id[subframe]]);

          if (ret <= UE->dlsch[UE->current_thread_id[subframe]][0][0]->max_turbo_iterations) {
            avg_iter += ret;
            iter_trials++;

            if (n_frames==1)
              printf("No DLSCH errors found (round %d),uncoded ber %f\n",round,uncoded_ber);

            UE->total_TBS[eNB_id] =  UE->total_TBS[eNB_id] + UE->dlsch[UE->current_thread_id[subframe]][eNB_id][0]->harq_processes[UE->dlsch[UE->current_thread_id[subframe]][eNB_id][0]->current_harq_pid]->TBS;
            TB0_active = 0;

            if (UE->dlsch[UE->current_thread_id[subframe]][eNB_id][0]->harq_processes[UE->dlsch[UE->current_thread_id[subframe]][eNB_id][0]->current_harq_pid]->mimo_mode ==
                LARGE_CDD) {   //try to decode second stream using SIC
              /*
              for (round = 0 ; round < UE->dlsch[UE->current_thread_id[subframe]][eNB_id][0]->harq_processes[UE->dlsch[UE->current_thread_id[subframe]][eNB_id][0]->current_harq_pid]->round ; round++) {
              // re-encoding of first stream
              dlsch0_ue_harq = UE->dlsch[UE->current_thread_id[subframe]][eNB_id][0]->harq_processes[UE->dlsch[UE->current_thread_id[subframe]][eNB_id][0]->current_harq_pid];
              dlsch0_eNB_harq = UE->dlsch[UE->current_thread_id[subframe]][eNB_id]->harq_processes[UE->dlsch[UE->current_thread_id[subframe]][eNB_id][0]->current_harq_pid];
              dlsch0_eNB_harq->mimo_mode    = LARGE_CDD;
              dlsch0_eNB_harq->rb_alloc[0]  = dlsch0_ue_harq->rb_alloc_even[0];
              dlsch0_eNB_harq->nb_rb        = dlsch0_ue_harq->nb_rb;
              dlsch0_eNB_harq->mcs          = dlsch0_ue_harq->mcs;
              dlsch0_eNB_harq->rvidx        = dlsch0_ue_harq->rvidx;
              dlsch0_eNB_harq->Nl           = dlsch0_ue_harq->Nl;

              dlsch0_eNB_harq->TBS          = dlsch0_ue_harq->TBS;
              dlsch0_eNB_harq->dl_power_off = dlsch0_ue_harq->dl_power_off;
              dlsch0_eNB_harq->status       = dlsch0_ue_harq->status;

              UE->dlsch[UE->current_thread_id[subframe]][eNB_id]->active       = UE->dlsch[UE->current_thread_id[subframe]][eNB_id][0]->active;
              UE->dlsch[UE->current_thread_id[subframe]][eNB_id]->rnti         = UE->dlsch[UE->current_thread_id[subframe]][eNB_id][0]->rnti;

              dlsch_encoding(UE->dlsch[UE->current_thread_id[subframe]][eNB_id][0]->harq_processes[UE->dlsch[UE->current_thread_id[subframe]][eNB_id][0]->current_harq_pid]->b,
                   &UE->frame_parms,
                   num_pdcch_symbols,
                   UE->dlsch[UE->current_thread_id[subframe]][0],
                   0,subframe,
                   &UE->dlsch_rate_matching_stats,
                   &UE->dlsch_turbo_encoding_stats,
                   &UE->dlsch_interleaving_stats
                   );

                   //scrambling

              //modulation

              //stripping (from matched filter output?)

              //detection of second stream
              }
              */
            }
          } else {
            errs[round]++;
            avg_iter += ret-1;
            iter_trials++;

            if (n_frames==1) {
              //if ((n_frames==1) || (SNR>=30))
              printf("DLSCH errors found (round %d), uncoded ber %f\n",round,uncoded_ber);

              for (s=0; s<UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[0]->C; s++) {
                if (s<UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[0]->Cminus)
                  Kr = UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[0]->Kminus;
                else
                  Kr = UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[0]->Kplus;

                Kr_bytes = Kr>>3;
                printf("Decoded_output (Segment %d):\n",s);

                for (i=0; i<Kr_bytes; i++)
                  printf("%d : %x (%x)\n",i,UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[0]->c[s][i],
                         UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[0]->c[s][i]^eNB->dlsch[0][0]->harq_processes[0]->c[s][i]);
              }

              sprintf(fname,"rxsig0_r%d.m",round);
              sprintf(vname,"rxs0_r%d",round);
              LOG_M(fname,vname, &UE->common_vars.rxdata[0][0],10*UE->frame_parms.samples_per_tti,1,1);
              sprintf(fname,"rxsigF0_r%d.m",round);
              sprintf(vname,"rxs0F_r%d",round);
              LOG_M(fname,vname, &UE->common_vars.common_vars_rx_data_per_thread[UE->current_thread_id[subframe]].rxdataF[0][0],2*UE->frame_parms.ofdm_symbol_size*nsymb,2,1);

              if (UE->frame_parms.nb_antennas_rx>1) {
                sprintf(fname,"rxsig1_r%d.m",round);
                sprintf(vname,"rxs1_r%d.m",round);
                LOG_M(fname,vname, UE->common_vars.rxdata[1],UE->frame_parms.samples_per_tti,1,1);
                sprintf(fname,"rxsig1F_r%d.m",round);
                sprintf(vname,"rxs1F_r%d.m",round);
                LOG_M(fname,vname, UE->common_vars.common_vars_rx_data_per_thread[UE->current_thread_id[subframe]].rxdataF[1],2*UE->frame_parms.ofdm_symbol_size*nsymb,2,1);
              }

              sprintf(fname,"dlsch00_r%d.m",round);
              sprintf(vname,"dl00_r%d",round);
              LOG_M(fname,vname,
                    &(UE->common_vars.common_vars_rx_data_per_thread[UE->current_thread_id[subframe]].dl_ch_estimates[eNB_id][0][0]),
                    UE->frame_parms.ofdm_symbol_size*nsymb,1,1);

              if (UE->frame_parms.nb_antennas_rx>1) {
                sprintf(fname,"dlsch01_r%d.m",round);
                sprintf(vname,"dl01_r%d",round);
                LOG_M(fname,vname,
                      &(UE->common_vars.common_vars_rx_data_per_thread[UE->current_thread_id[subframe]].dl_ch_estimates[eNB_id][1][0]),
                      UE->frame_parms.ofdm_symbol_size*nsymb/2,1,1);
              }

              if (eNB->frame_parms.nb_antennas_tx>1) {
                sprintf(fname,"dlsch10_r%d.m",round);
                sprintf(vname,"dl10_r%d",round);
                LOG_M(fname,vname,
                      &(UE->common_vars.common_vars_rx_data_per_thread[UE->current_thread_id[subframe]].dl_ch_estimates[eNB_id][2][0]),
                      UE->frame_parms.ofdm_symbol_size*nsymb/2,1,1);
              }

              if ((UE->frame_parms.nb_antennas_rx>1) && (eNB->frame_parms.nb_antennas_tx>1)) {
                sprintf(fname,"dlsch11_r%d.m",round);
                sprintf(vname,"dl11_r%d",round);
                LOG_M(fname,vname,
                      &(UE->common_vars.common_vars_rx_data_per_thread[UE->current_thread_id[subframe]].dl_ch_estimates[eNB_id][3][0]),
                      UE->frame_parms.ofdm_symbol_size*nsymb/2,1,1);
              }

              //pdsch_vars
              dump_dlsch2(UE,eNB_id,subframe,&coded_bits_per_codeword,round);
              /*
              LOG_M("dlsch_e.m","e",eNB->dlsch[0][0]->harq_processes[0]->e,coded_bits_per_codeword,1,4);
              LOG_M("dlsch_ber_bit.m","ber_bit",uncoded_ber_bit,coded_bits_per_codeword,1,0);
              LOG_M("dlsch_w.m","w",eNB->dlsch[0][0]->harq_processes[0]->w[0],3*(tbs+64),1,4);
              LOG_M("dlsch_w.m","w",UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[0]->w[0],3*(tbs+64),1,0);
              */

              if (round == 3) exit(-1);
            }

            //      printf("round %d errors %d/%d\n",round,errs[round],trials);
            round++;
            //      UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[0]->round++;
          }

          if (xforms==1) {
            phy_scope_UE(form_ue,
                         UE,
                         eNB_id,
                         0,// UE_id
                         subframe);
          }
        }  //round

        //      printf("\n");

        if ((errs[0]>=n_frames/10) && (trials>(n_frames/2)))
          break;

        //len = chbch_stats_read(stats_buffer,NULL,0,4096);
        //printf("%s\n\n",stats_buffer);

        if (UE->proc.proc_rxtx[UE->current_thread_id[subframe]].frame_rx % 10 == 0) {
          UE->bitrate[eNB_id] = (UE->total_TBS[eNB_id] - UE->total_TBS_last[eNB_id])*10;
          LOG_D(PHY,"[UE %d] Calculating bitrate: total_TBS = %d, total_TBS_last = %d, bitrate = %d kbits/s\n",UE->Mod_id,UE->total_TBS[eNB_id],UE->total_TBS_last[eNB_id],
                UE->bitrate[eNB_id]/1000);
          UE->total_TBS_last[eNB_id] = UE->total_TBS[eNB_id];
        }

        UE->proc.proc_rxtx[UE->current_thread_id[subframe]].frame_rx++;
        /* calculate the total processing time for each packet,
         * get the max, min, and number of packets that exceed t>2000us
         */
        double t_tx = (double)eNB->phy_proc_tx.p_time/cpu_freq_GHz/1000.0;
        double t_tx_ifft = (double)eNB->ofdm_mod_stats.p_time/cpu_freq_GHz/1000.0;
        double t_tx_mod = (double)eNB->dlsch_modulation_stats.p_time/cpu_freq_GHz/1000.0;
        double t_tx_enc = (double)eNB->dlsch_encoding_stats.p_time/cpu_freq_GHz/1000.0;
        double t_rx = (double)UE->phy_proc_rx[UE->current_thread_id[subframe]].p_time/cpu_freq_GHz/1000.0;
        double t_rx_fft = (double)UE->ofdm_demod_stats.p_time/cpu_freq_GHz/1000.0;
        double t_rx_demod = (double)UE->dlsch_rx_pdcch_stats.p_time/cpu_freq_GHz/1000.0;
        double t_rx_dec = (double)UE->dlsch_decoding_stats.p_time/cpu_freq_GHz/1000.0;

        if (t_tx > t_tx_max)
          t_tx_max = t_tx;

        if (t_tx < t_tx_min)
          t_tx_min = t_tx;

        if (t_rx > t_rx_max)
          t_rx_max = t_rx;

        if (t_rx < t_rx_min)
          t_rx_min = t_rx;

        if (t_tx > 2000)
          n_tx_dropped++;

        if (t_rx > 2000)
          n_rx_dropped++;

        push_front(&time_vector_tx, t_tx);
        push_front(&time_vector_tx_ifft, t_tx_ifft);
        push_front(&time_vector_tx_mod, t_tx_mod);
        push_front(&time_vector_tx_enc, t_tx_enc);
        push_front(&time_vector_rx, t_rx);
        push_front(&time_vector_rx_fft, t_rx_fft);
        push_front(&time_vector_rx_demod, t_rx_demod);
        push_front(&time_vector_rx_dec, t_rx_dec);
      }   //trials

      // round_trials[0]: number of code word : goodput the protocol
      double table_tx[time_vector_tx.size];
      totable(table_tx, &time_vector_tx);
      double table_tx_ifft[time_vector_tx_ifft.size];
      totable(table_tx_ifft, &time_vector_tx_ifft);
      double table_tx_mod[time_vector_tx_mod.size];
      totable(table_tx_mod, &time_vector_tx_mod);
      double table_tx_enc[time_vector_tx_enc.size];
      totable(table_tx_enc, &time_vector_tx_enc);
      double table_rx[time_vector_rx.size];
      totable(table_rx, &time_vector_rx);
      double table_rx_fft[time_vector_rx_fft.size];
      totable(table_rx_fft, &time_vector_rx_fft);
      double table_rx_demod[time_vector_rx_demod.size];
      totable(table_rx_demod, &time_vector_rx_demod);
      double table_rx_dec[time_vector_rx_dec.size];
      totable(table_rx_dec, &time_vector_rx_dec);
      // sort table
      qsort (table_tx, time_vector_tx.size, sizeof(double), &compare);
      qsort (table_rx, time_vector_rx.size, sizeof(double), &compare);

      if (dump_table == 1 ) {
        set_component_filelog(SIM);  // file located in /tmp/usim.txt
        LOG_UDUMPMSG(SIM,table_tx,time_vector_tx.size,LOG_DUMP_DOUBLE,"The transmitter raw data: \n");
        LOG_UDUMPMSG(SIM,table_rx,time_vector_rx.size,LOG_DUMP_DOUBLE,"The receiver raw data: \n");
      }

      double tx_median = table_tx[time_vector_tx.size/2];
      double tx_q1 = table_tx[time_vector_tx.size/4];
      double tx_q3 = table_tx[3*time_vector_tx.size/4];
      double tx_ifft_median = table_tx_ifft[time_vector_tx_ifft.size/2];
      double tx_ifft_q1 = table_tx_ifft[time_vector_tx_ifft.size/4];
      double tx_ifft_q3 = table_tx_ifft[3*time_vector_tx_ifft.size/4];
      double tx_mod_median = table_tx_mod[time_vector_tx_mod.size/2];
      double tx_mod_q1 = table_tx_mod[time_vector_tx_mod.size/4];
      double tx_mod_q3 = table_tx_mod[3*time_vector_tx_mod.size/4];
      double tx_enc_median = table_tx_enc[time_vector_tx_enc.size/2];
      double tx_enc_q1 = table_tx_enc[time_vector_tx_enc.size/4];
      double tx_enc_q3 = table_tx_enc[3*time_vector_tx_enc.size/4];
      double rx_median = table_rx[time_vector_rx.size/2];
      double rx_q1 = table_rx[time_vector_rx.size/4];
      double rx_q3 = table_rx[3*time_vector_rx.size/4];
      double rx_fft_median = table_rx_fft[time_vector_rx_fft.size/2];
      double rx_fft_q1 = table_rx_fft[time_vector_rx_fft.size/4];
      double rx_fft_q3 = table_rx_fft[3*time_vector_rx_fft.size/4];
      double rx_demod_median = table_rx_demod[time_vector_rx_demod.size/2];
      double rx_demod_q1 = table_rx_demod[time_vector_rx_demod.size/4];
      double rx_demod_q3 = table_rx_demod[3*time_vector_rx_demod.size/4];
      double rx_dec_median = table_rx_dec[time_vector_rx_dec.size/2];
      double rx_dec_q1 = table_rx_dec[time_vector_rx_dec.size/4];
      double rx_dec_q3 = table_rx_dec[3*time_vector_rx_dec.size/4];
      double std_phy_proc_tx=0;
      double std_phy_proc_tx_ifft=0;
      double std_phy_proc_tx_mod=0;
      double std_phy_proc_tx_enc=0;
      double std_phy_proc_rx=0;
      double std_phy_proc_rx_fft=0;
      double std_phy_proc_rx_demod=0;
      double std_phy_proc_rx_dec=0;
      effective_rate = ((double)(round_trials[0]-dci_errors)/((double)round_trials[0] + round_trials[1] + round_trials[2] + round_trials[3]));
      printf("\n**********************SNR = %f dB (tx_lev %f, sigma2_dB %f)**************************\n",
             SNR,
             (double)tx_lev_dB+10*log10(UE->frame_parms.ofdm_symbol_size/(NB_RB*12)),
             sigma2_dB);
      printf("Errors (%u(%u)/%u %u/%u %u/%u %u/%u), Pe = (%e,%e,%e,%e), dci_errors %u/%u, Pe = %e => effective rate %f  (%2.1f%%,%f, %f), normalized delay %f (%f)\n",
             errs[0],
             errs2[0],
             round_trials[0],
             errs[1],
             round_trials[0],
             errs[2],
             round_trials[0],
             errs[3],
             round_trials[0],
             (double)errs[0]/(round_trials[0]),
             (double)errs[1]/(round_trials[0]),
             (double)errs[2]/(round_trials[0]),
             (double)errs[3]/(round_trials[0]),
             dci_errors,
             round_trials[0],
             (double)dci_errors/(round_trials[0]),
             rate*effective_rate,
             100*effective_rate,
             rate,
             rate*get_Qm(UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[UE->dlsch[UE->current_thread_id[subframe]][0][0]->current_harq_pid]->mcs),
             (1.0*(round_trials[0]-errs[0])+2.0*(round_trials[1]-errs[1])+3.0*(round_trials[2]-errs[2])+4.0*(round_trials[3]-errs[3]))/((double)round_trials[0])/
             (double)eNB->dlsch[0][0]->harq_processes[0]->TBS,
             (1.0*(round_trials[0]-errs[0])+2.0*(round_trials[1]-errs[1])+3.0*(round_trials[2]-errs[2])+4.0*(round_trials[3]-errs[3]))/((double)round_trials[0]));

      if (print_perf==1) {
        printf("eNB TX function statistics (per 1ms subframe)\n\n");
        std_phy_proc_tx = sqrt((double)eNB->phy_proc_tx.diff_square/pow(cpu_freq_GHz,2)/pow(1000,
                               2)/eNB->phy_proc_tx.trials - pow((double)eNB->phy_proc_tx.diff/eNB->phy_proc_tx.trials/cpu_freq_GHz/1000,2));
        printf("Total PHY proc tx                 :%f us (%d trials)\n",(double)eNB->phy_proc_tx.diff/eNB->phy_proc_tx.trials/cpu_freq_GHz/1000.0,eNB->phy_proc_tx.trials);
        printf("|__ Statistcs                           std: %fus max: %fus min: %fus median %fus q1 %fus q3 %fus n_dropped: %d packet \n",std_phy_proc_tx, t_tx_max, t_tx_min, tx_median, tx_q1, tx_q3,
               n_tx_dropped);
        std_phy_proc_tx_ifft = sqrt((double)eNB->ofdm_mod_stats.diff_square/pow(cpu_freq_GHz,2)/pow(1000,
                                    2)/eNB->ofdm_mod_stats.trials - pow((double)eNB->ofdm_mod_stats.diff/eNB->ofdm_mod_stats.trials/cpu_freq_GHz/1000,2));
        printf("OFDM_mod time                     :%f us (%d trials)\n",(double)eNB->ofdm_mod_stats.diff/eNB->ofdm_mod_stats.trials/cpu_freq_GHz/1000.0,eNB->ofdm_mod_stats.trials);
        printf("|__ Statistcs                           std: %fus median %fus q1 %fus q3 %fus \n",std_phy_proc_tx_ifft, tx_ifft_median, tx_ifft_q1, tx_ifft_q3);
        std_phy_proc_tx_mod = sqrt((double)eNB->dlsch_modulation_stats.diff_square/pow(cpu_freq_GHz,2)/pow(1000,
                                   2)/eNB->dlsch_modulation_stats.trials - pow((double)eNB->dlsch_modulation_stats.diff/eNB->dlsch_modulation_stats.trials/cpu_freq_GHz/1000,2));
        printf("DLSCH modulation time             :%f us (%d trials)\n",(double)eNB->dlsch_modulation_stats.diff/eNB->dlsch_modulation_stats.trials/cpu_freq_GHz/1000.0,
               eNB->dlsch_modulation_stats.trials);
        printf("|__ Statistcs                           std: %fus median %fus q1 %fus q3 %fus \n",std_phy_proc_tx_mod, tx_mod_median, tx_mod_q1, tx_mod_q3);
        printf("DLSCH scrambling time             :%f us (%d trials)\n",(double)eNB->dlsch_scrambling_stats.diff/eNB->dlsch_scrambling_stats.trials/cpu_freq_GHz/1000.0,
               eNB->dlsch_scrambling_stats.trials);
        std_phy_proc_tx_enc = sqrt((double)eNB->dlsch_encoding_stats.diff_square/pow(cpu_freq_GHz,2)/pow(1000,
                                   2)/eNB->dlsch_encoding_stats.trials - pow((double)eNB->dlsch_encoding_stats.diff/eNB->dlsch_encoding_stats.trials/cpu_freq_GHz/1000,2));
        printf("DLSCH encoding time               :%f us (%d trials)\n",(double)eNB->dlsch_encoding_stats.diff/eNB->dlsch_encoding_stats.trials/cpu_freq_GHz/1000.0,
               eNB->dlsch_modulation_stats.trials);
        printf("|__ Statistcs                           std: %fus median %fus q1 %fus q3 %fus \n",std_phy_proc_tx_enc, tx_enc_median, tx_enc_q1, tx_enc_q3);
        printf("|__ DLSCH turbo encoding time         :%f us (%d trials)\n",
               ((double)eNB->dlsch_turbo_encoding_stats.trials/eNB->dlsch_encoding_stats.trials)*(double)
               eNB->dlsch_turbo_encoding_stats.diff/eNB->dlsch_turbo_encoding_stats.trials/cpu_freq_GHz/1000.0,eNB->dlsch_turbo_encoding_stats.trials);
        printf("|__ DLSCH rate-matching time          :%f us (%d trials)\n",
               ((double)eNB->dlsch_rate_matching_stats.trials/eNB->dlsch_encoding_stats.trials)*(double)
               eNB->dlsch_rate_matching_stats.diff/eNB->dlsch_rate_matching_stats.trials/cpu_freq_GHz/1000.0,eNB->dlsch_rate_matching_stats.trials);
        printf("|__ DLSCH sub-block interleaving time :%f us (%d trials)\n",
               ((double)eNB->dlsch_interleaving_stats.trials/eNB->dlsch_encoding_stats.trials)*(double)
               eNB->dlsch_interleaving_stats.diff/eNB->dlsch_interleaving_stats.trials/cpu_freq_GHz/1000.0,eNB->dlsch_interleaving_stats.trials);
        printf("\n\nUE RX function statistics (per 1ms subframe)\n\n");
        std_phy_proc_rx = sqrt((double)UE->phy_proc_rx[UE->current_thread_id[subframe]].diff_square/pow(cpu_freq_GHz,2)/pow(1000,
                               2)/UE->phy_proc_rx[UE->current_thread_id[subframe]].trials - pow((double)
                                   UE->phy_proc_rx[UE->current_thread_id[subframe]].diff/UE->phy_proc_rx[UE->current_thread_id[subframe]].trials/cpu_freq_GHz/1000,2));
        printf("Total PHY proc rx                                   :%f us (%d trials)\n",
               (double)UE->phy_proc_rx[UE->current_thread_id[subframe]].diff/UE->phy_proc_rx[UE->current_thread_id[subframe]].trials/cpu_freq_GHz/1000.0,
               UE->phy_proc_rx[UE->current_thread_id[subframe]].trials*2/3);
        printf("|__Statistcs                                            std: %fus max: %fus min: %fus median %fus q1 %fus q3 %fus n_dropped: %d packet \n", std_phy_proc_rx, t_rx_max, t_rx_min, rx_median,
               rx_q1, rx_q3, n_rx_dropped);
        std_phy_proc_rx_fft = sqrt((double)UE->ofdm_demod_stats.diff_square/pow(cpu_freq_GHz,2)/pow(1000,
                                   2)/UE->ofdm_demod_stats.trials - pow((double)UE->ofdm_demod_stats.diff/UE->ofdm_demod_stats.trials/cpu_freq_GHz/1000,2));
        printf("DLSCH OFDM demodulation and channel_estimation time :%f us (%d trials)\n",(nsymb)*(double)UE->ofdm_demod_stats.diff/UE->ofdm_demod_stats.trials/cpu_freq_GHz/1000.0,
               UE->ofdm_demod_stats.trials*2/3);
        printf("|__ Statistcs                           std: %fus median %fus q1 %fus q3 %fus \n",std_phy_proc_rx_fft, rx_fft_median, rx_fft_q1, rx_fft_q3);
        printf("|__ DLSCH rx dft                                        :%f us (%d trials)\n",
               (nsymb*UE->frame_parms.nb_antennas_rx)*(double)UE->rx_dft_stats.diff/UE->rx_dft_stats.trials/cpu_freq_GHz/1000.0,UE->rx_dft_stats.trials*2/3);
        printf("|__ DLSCH channel estimation time                       :%f us (%d trials)\n",
               (4.0)*(double)UE->dlsch_channel_estimation_stats.diff/UE->dlsch_channel_estimation_stats.trials/cpu_freq_GHz/1000.0,UE->dlsch_channel_estimation_stats.trials*2/3);
        printf("|__ DLSCH frequency offset estimation time              :%f us (%d trials)\n",
               (4.0)*(double)UE->dlsch_freq_offset_estimation_stats.diff/UE->dlsch_freq_offset_estimation_stats.trials/cpu_freq_GHz/1000.0,
               UE->dlsch_freq_offset_estimation_stats.trials*2/3);
        printf("DLSCH rx pdcch                                       :%f us (%d trials)\n",(double)UE->dlsch_rx_pdcch_stats.diff/UE->dlsch_rx_pdcch_stats.trials/cpu_freq_GHz/1000.0,
               UE->dlsch_rx_pdcch_stats.trials);
        std_phy_proc_rx_demod = sqrt((double)UE->dlsch_llr_stats.diff_square/pow(cpu_freq_GHz,2)/pow(1000,
                                     2)/UE->dlsch_llr_stats.trials - pow((double)UE->dlsch_llr_stats.diff/UE->dlsch_llr_stats.trials/cpu_freq_GHz/1000,2));
        printf("DLSCH Channel Compensation and LLR generation time  :%f us (%d trials)\n",(3)*(double)UE->dlsch_llr_stats.diff/UE->dlsch_llr_stats.trials/cpu_freq_GHz/1000.0,
               UE->dlsch_llr_stats.trials/3);
        printf("|__ Statistcs                           std: %fus median %fus q1 %fus q3 %fus \n",std_phy_proc_rx_demod, rx_demod_median, rx_demod_q1, rx_demod_q3);
        printf("DLSCH unscrambling time                             :%f us (%d trials)\n",(double)UE->dlsch_unscrambling_stats.diff/UE->dlsch_unscrambling_stats.trials/cpu_freq_GHz/1000.0,
               UE->dlsch_unscrambling_stats.trials);
        std_phy_proc_rx_dec = sqrt((double)UE->dlsch_decoding_stats.diff_square/pow(cpu_freq_GHz,2)/pow(1000,
                                   2)/UE->dlsch_decoding_stats.trials - pow((double)UE->dlsch_decoding_stats.diff/UE->dlsch_decoding_stats.trials/cpu_freq_GHz/1000,2));
        printf("DLSCH Decoding time (%02.2f Mbit/s, avg iter %1.2f)    :%f us (%d trials, max %f)\n",
               eNB->dlsch[0][0]->harq_processes[0]->TBS/1000.0,(double)avg_iter/iter_trials,
               (double)UE->dlsch_decoding_stats.diff/UE->dlsch_decoding_stats.trials/cpu_freq_GHz/1000.0,UE->dlsch_decoding_stats.trials,
               (double)UE->dlsch_decoding_stats.max/cpu_freq_GHz/1000.0);
        printf("|__ Statistcs                           std: %fus median %fus q1 %fus q3 %fus \n",std_phy_proc_rx_dec, rx_dec_median, rx_dec_q1, rx_dec_q3);
        printf("|__ DLSCH Rate Unmatching                               :%f us (%d trials)\n",
               (double)UE->dlsch_rate_unmatching_stats.diff/UE->dlsch_rate_unmatching_stats.trials/cpu_freq_GHz/1000.0,UE->dlsch_rate_unmatching_stats.trials);
        printf("|__ DLSCH Turbo Decoding(%d bits)                       :%f us (%d trials)\n",
               UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[0]->Cminus ? UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[0]->Kminus :
               UE->dlsch[UE->current_thread_id[subframe]][0][0]->harq_processes[0]->Kplus,
               (double)UE->dlsch_turbo_decoding_stats.diff/UE->dlsch_turbo_decoding_stats.trials/cpu_freq_GHz/1000.0,UE->dlsch_turbo_decoding_stats.trials);
        printf("    |__ init                                            %f us (cycles/iter %f, %d trials)\n",
               (double)UE->dlsch_tc_init_stats.diff/UE->dlsch_tc_init_stats.trials/cpu_freq_GHz/1000.0,
               (double)UE->dlsch_tc_init_stats.diff/UE->dlsch_tc_init_stats.trials/((double)avg_iter/iter_trials),
               UE->dlsch_tc_init_stats.trials);
        printf("    |__ alpha                                           %f us (cycles/iter %f, %d trials)\n",
               (double)UE->dlsch_tc_alpha_stats.diff/UE->dlsch_tc_alpha_stats.trials/cpu_freq_GHz/1000.0,
               (double)UE->dlsch_tc_alpha_stats.diff/UE->dlsch_tc_alpha_stats.trials*2,
               UE->dlsch_tc_alpha_stats.trials);
        printf("    |__ beta                                            %f us (cycles/iter %f,%d trials)\n",
               (double)UE->dlsch_tc_beta_stats.diff/UE->dlsch_tc_beta_stats.trials/cpu_freq_GHz/1000.0,
               (double)UE->dlsch_tc_beta_stats.diff/UE->dlsch_tc_beta_stats.trials*2,
               UE->dlsch_tc_beta_stats.trials);
        printf("    |__ gamma                                           %f us (cycles/iter %f,%d trials)\n",
               (double)UE->dlsch_tc_gamma_stats.diff/UE->dlsch_tc_gamma_stats.trials/cpu_freq_GHz/1000.0,
               (double)UE->dlsch_tc_gamma_stats.diff/UE->dlsch_tc_gamma_stats.trials*2,
               UE->dlsch_tc_gamma_stats.trials);
        printf("    |__ ext                                             %f us (cycles/iter %f,%d trials)\n",
               (double)UE->dlsch_tc_ext_stats.diff/UE->dlsch_tc_ext_stats.trials/cpu_freq_GHz/1000.0,
               (double)UE->dlsch_tc_ext_stats.diff/UE->dlsch_tc_ext_stats.trials*2,
               UE->dlsch_tc_ext_stats.trials);
        printf("    |__ intl1                                           %f us (cycles/iter %f,%d trials)\n",
               (double)UE->dlsch_tc_intl1_stats.diff/UE->dlsch_tc_intl1_stats.trials/cpu_freq_GHz/1000.0,
               (double)UE->dlsch_tc_intl1_stats.diff/UE->dlsch_tc_intl1_stats.trials,
               UE->dlsch_tc_intl1_stats.trials);
        printf("    |__ intl2+HD+CRC                                    %f us (cycles/iter %f,%d trials)\n",
               (double)UE->dlsch_tc_intl2_stats.diff/UE->dlsch_tc_intl2_stats.trials/cpu_freq_GHz/1000.0,
               (double)UE->dlsch_tc_intl2_stats.diff/UE->dlsch_tc_intl2_stats.trials,
               UE->dlsch_tc_intl2_stats.trials);
      }

      if ((transmission_mode != 3) && (transmission_mode != 4)) {
        fprintf(bler_fd,"%f;%d;%d;%f;%u;%u;%u;%u;%u;%u;%u;%u;%u\n",
                SNR,
                mcs1,
                eNB->dlsch[0][0]->harq_processes[0]->TBS,
                rate,
                errs[0],
                round_trials[0],
                errs[1],
                round_trials[1],
                errs[2],
                round_trials[2],
                errs[3],
                round_trials[3],
                dci_errors);
      } else {
        fprintf(bler_fd,"%f;%d;%d;%d;%d;%f;%u;%u;%u;%u;%u;%u;%u;%u;%u\n",
                SNR,
                mcs1,mcs2,
                eNB->dlsch[0][0]->harq_processes[0]->TBS,
                eNB->dlsch[0][1]->harq_processes[0]->TBS,
                rate,
                errs[0],
                round_trials[0],
                errs[1],
                round_trials[1],
                errs[2],
                round_trials[2],
                errs[3],
                round_trials[3],
                dci_errors);
      }

      if(abstx) { //ABSTRACTION
        blerr[0] = (double)errs[0]/(round_trials[0]);

        if(num_rounds>1) {
          blerr[1] = (double)errs[1]/(round_trials[1]);
          blerr[2] = (double)errs[2]/(round_trials[2]);
          blerr[3] = (double)errs[3]/(round_trials[3]);
          fprintf(csv_fd,"%e,%e,%e,%e;\n",blerr[0],blerr[1],blerr[2],blerr[3]);
        } else {
          fprintf(csv_fd,"%e;\n",blerr[0]);
        }
      } //ABStraction

      if ( (test_perf != 0) && (100 * effective_rate > test_perf )) {
        //fprintf(time_meas_fd,"SNR; MCS; TBS; rate; err0; trials0; err1; trials1; err2; trials2; err3; trials3; dci_err\n");
        if ((transmission_mode != 3) && (transmission_mode != 4)) {
          fprintf(time_meas_fd,"%f;%d;%d;%f;%u;%u;%u;%u;%u;%u;%u;%u;%u;",
                  SNR,
                  mcs1,
                  eNB->dlsch[0][0]->harq_processes[0]->TBS,
                  rate,
                  errs[0],
                  round_trials[0],
                  errs[1],
                  round_trials[1],
                  errs[2],
                  round_trials[2],
                  errs[3],
                  round_trials[3],
                  dci_errors);
          //fprintf(time_meas_fd,"SNR; MCS; TBS; rate; DL_DECOD_ITER; err0; trials0; err1; trials1; err2; trials2; err3; trials3; PE; dci_err;PE;ND;\n");
          fprintf(time_meas_fd,"%f;%d;%d;%f; %2.1f%%;%f;%f;%u;%u;%u;%u;%u;%u;%u;%u;%e;%e;%e;%e;%u;%u;%e;%f;%f;",
                  SNR,
                  mcs1,
                  eNB->dlsch[0][0]->harq_processes[0]->TBS,
                  rate*effective_rate,
                  100*effective_rate,
                  rate,
                  (double)avg_iter/iter_trials,
                  errs[0],
                  round_trials[0],
                  errs[1],
                  round_trials[1],
                  errs[2],
                  round_trials[2],
                  errs[3],
                  round_trials[3],
                  (double)errs[0]/(round_trials[0]),
                  (double)errs[1]/(round_trials[0]),
                  (double)errs[2]/(round_trials[0]),
                  (double)errs[3]/(round_trials[0]),
                  dci_errors,
                  round_trials[0],
                  (double)dci_errors/(round_trials[0]),
                  (1.0*(round_trials[0]-errs[0])+2.0*(round_trials[1]-errs[1])+3.0*(round_trials[2]-errs[2])+4.0*(round_trials[3]-errs[3]))/((double)round_trials[0])/
                  (double)eNB->dlsch[0][0]->harq_processes[0]->TBS,
                  (1.0*(round_trials[0]-errs[0])+2.0*(round_trials[1]-errs[1])+3.0*(round_trials[2]-errs[2])+4.0*(round_trials[3]-errs[3]))/((double)round_trials[0]));
        } else {
          fprintf(time_meas_fd,"%f;%d;%d;%d;%d;%f;%u;%u;%u;%u;%u;%u;%u;%u;%u;",
                  SNR,
                  mcs1,mcs2,
                  eNB->dlsch[0][0]->harq_processes[0]->TBS,
                  eNB->dlsch[0][1]->harq_processes[0]->TBS,
                  rate,
                  errs[0],
                  round_trials[0],
                  errs[1],
                  round_trials[1],
                  errs[2],
                  round_trials[2],
                  errs[3],
                  round_trials[3],
                  dci_errors);
          //fprintf(time_meas_fd,"SNR; MCS; TBS; rate; DL_DECOD_ITER; err0; trials0; err1; trials1; err2; trials2; err3; trials3; PE; dci_err;PE;ND;\n");
          fprintf(time_meas_fd,"%f;%d;%d;%d;%d;%f;%2.1f;%f;%f;%u;%u;%u;%u;%u;%u;%u;%u;%e;%e;%e;%e;%u;%u;%e;%f;%f;",
                  SNR,
                  mcs1,mcs2,
                  eNB->dlsch[0][0]->harq_processes[0]->TBS,
                  eNB->dlsch[0][1]->harq_processes[0]->TBS,
                  rate*effective_rate,
                  100*effective_rate,
                  rate,
                  (double)avg_iter/iter_trials,
                  errs[0],
                  round_trials[0],
                  errs[1],
                  round_trials[1],
                  errs[2],
                  round_trials[2],
                  errs[3],
                  round_trials[3],
                  (double)errs[0]/(round_trials[0]),
                  (double)errs[1]/(round_trials[0]),
                  (double)errs[2]/(round_trials[0]),
                  (double)errs[3]/(round_trials[0]),
                  dci_errors,
                  round_trials[0],
                  (double)dci_errors/(round_trials[0]),
                  (1.0*(round_trials[0]-errs[0])+2.0*(round_trials[1]-errs[1])+3.0*(round_trials[2]-errs[2])+4.0*(round_trials[3]-errs[3]))/((double)round_trials[0])/
                  (double)eNB->dlsch[0][0]->harq_processes[0]->TBS,
                  (1.0*(round_trials[0]-errs[0])+2.0*(round_trials[1]-errs[1])+3.0*(round_trials[2]-errs[2])+4.0*(round_trials[3]-errs[3]))/((double)round_trials[0]));
        }

        //fprintf(time_meas_fd,"eNB_PROC_TX(%d); OFDM_MOD(%d); DL_MOD(%d); DL_SCR(%d); DL_ENC(%d); UE_PROC_RX(%d); OFDM_DEMOD_CH_EST(%d); RX_PDCCH(%d); CH_COMP_LLR(%d); DL_USCR(%d); DL_DECOD(%d);\n",
        fprintf(time_meas_fd,"%d; %d; %d; %d; %d; %d; %d; %d; %d; %d; %d;",
                eNB->phy_proc_tx.trials,
                eNB->ofdm_mod_stats.trials,
                eNB->dlsch_modulation_stats.trials,
                eNB->dlsch_scrambling_stats.trials,
                eNB->dlsch_encoding_stats.trials,
                UE->phy_proc_rx[UE->current_thread_id[subframe]].trials,
                UE->ofdm_demod_stats.trials,
                UE->dlsch_rx_pdcch_stats.trials,
                UE->dlsch_llr_stats.trials,
                UE->dlsch_unscrambling_stats.trials,
                UE->dlsch_decoding_stats.trials
               );
        fprintf(time_meas_fd,"%f;%f;%f;%f;%f;%f;%f;%f;%f;%f;%f;",
                get_time_meas_us(&eNB->phy_proc_tx),
                get_time_meas_us(&eNB->ofdm_mod_stats),
                get_time_meas_us(&eNB->dlsch_modulation_stats),
                get_time_meas_us(&eNB->dlsch_scrambling_stats),
                get_time_meas_us(&eNB->dlsch_encoding_stats),
                get_time_meas_us(&UE->phy_proc_rx[UE->current_thread_id[subframe]]),
                nsymb*get_time_meas_us(&UE->ofdm_demod_stats),
                get_time_meas_us(&UE->dlsch_rx_pdcch_stats),
                3*get_time_meas_us(&UE->dlsch_llr_stats),
                get_time_meas_us(&UE->dlsch_unscrambling_stats),
                get_time_meas_us(&UE->dlsch_decoding_stats)
               );
        //fprintf(time_meas_fd,"eNB_PROC_TX_STD;eNB_PROC_TX_MAX;eNB_PROC_TX_MIN;eNB_PROC_TX_MED;eNB_PROC_TX_Q1;eNB_PROC_TX_Q3;eNB_PROC_TX_DROPPED;\n");
        fprintf(time_meas_fd,"%f;%f;%f;%f;%f;%f;%d;", std_phy_proc_tx, t_tx_max, t_tx_min, tx_median, tx_q1, tx_q3, n_tx_dropped);
        //fprintf(time_meas_fd,"IFFT;\n");
        fprintf(time_meas_fd,"%f;%f;%f;%f;", std_phy_proc_tx_ifft, tx_ifft_median, tx_ifft_q1, tx_ifft_q3);
        //fprintf(time_meas_fd,"MOD;\n");
        fprintf(time_meas_fd,"%f;%f;%f;%f;", std_phy_proc_tx_mod, tx_mod_median, tx_mod_q1, tx_mod_q3);
        //fprintf(time_meas_fd,"ENC;\n");
        fprintf(time_meas_fd,"%f;%f;%f;%f;", std_phy_proc_tx_enc, tx_enc_median, tx_enc_q1, tx_enc_q3);
        //fprintf(time_meas_fd,"UE_PROC_RX_STD;UE_PROC_RX_MAX;UE_PROC_RX_MIN;UE_PROC_RX_MED;UE_PROC_RX_Q1;UE_PROC_RX_Q3;UE_PROC_RX_DROPPED;\n");
        fprintf(time_meas_fd,"%f;%f;%f;%f;%f;%f;%d;", std_phy_proc_rx, t_rx_max, t_rx_min, rx_median, rx_q1, rx_q3, n_rx_dropped);
        //fprintf(time_meas_fd,"FFT;\n");
        fprintf(time_meas_fd,"%f;%f;%f;%f;", std_phy_proc_rx_fft, rx_fft_median, rx_fft_q1, rx_fft_q3);
        //fprintf(time_meas_fd,"DEMOD;\n");
        fprintf(time_meas_fd,"%f;%f;%f;%f;", std_phy_proc_rx_demod,rx_demod_median, rx_demod_q1, rx_demod_q3);
        //fprintf(time_meas_fd,"DEC;\n");
        fprintf(time_meas_fd,"%f;%f;%f;%f\n", std_phy_proc_rx_dec, rx_dec_median, rx_dec_q1, rx_dec_q3);
        /*
        fprintf(time_meas_fd,"%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;",
        eNB->phy_proc_tx.trials,
        eNB->ofdm_mod_stats.trials,
        eNB->dlsch_modulation_stats.trials,
        eNB->dlsch_scrambling_stats.trials,
        eNB->dlsch_encoding_stats.trials,
        UE->phy_proc_rx[UE->current_thread_id[subframe]].trials,
        UE->ofdm_demod_stats.trials,
        UE->dlsch_rx_pdcch_stats.trials,
        UE->dlsch_llr_stats.trials,
        UE->dlsch_unscrambling_stats.trials,
        UE->dlsch_decoding_stats.trials);
        */
        printf("[passed] effective rate : %f  (%2.1f%%,%f)): log and break \n",rate*effective_rate, 100*effective_rate, rate );
        break;
      } else if (test_perf !=0 ) {
        printf("[continue] effective rate : %f  (%2.1f%%,%f)): increase snr \n",rate*effective_rate, 100*effective_rate, rate);
      }

      if (((double)errs[0]/(round_trials[0]))<(10.0/n_frames))
        break;
    }// SNR
  } //ch_realization

  fclose(bler_fd);

  if (test_perf !=0)
    fclose (time_meas_fd);

  //fprintf(tikz_fd,"};\n");
  //fclose(tikz_fd);

  if (input_trch_file==1)
    fclose(input_trch_fd);

  if (input_file==1)
    fclose(input_fd);

  if(abstx) { // ABSTRACTION
    fprintf(csv_fd,"];");
    fclose(csv_fd);
  }

  if (uncoded_ber_bit)
    free(uncoded_ber_bit);

  uncoded_ber_bit = NULL;

  for (k=0; k<n_users; k++) {
    free(input_buffer0[k]);
    free(input_buffer1[k]);
    input_buffer0[k]=NULL;
    input_buffer1[k]=NULL;
  }

  printf("Freeing dlsch structures\n");

  for (i=0; i<2; i++) {
    printf("eNB %d\n",i);
    free_eNB_dlsch(eNB->dlsch[0][i]);
    printf("UE %d\n",i);
    free_ue_dlsch(UE->dlsch[UE->current_thread_id[subframe]][0][i]);
  }

  printf("Freeing channel I/O\n");

  for (i=0; i<n_tx_phy; i++) {
    free(s_re[i]);
    free(s_im[i]);
  }

  for (i=0; i<2; i++) {
    free(r_re[i]);
    free(r_im[i]);
  }

  free(s_re);
  free(s_im);
  free(r_re);
  free(r_im);
  //  lte_sync_time_free();
  //  printf("[MUMIMO] mcs %d, mcsi %d, offset %d, bler %f\n",mcs,mcs_i,offset_mumimo_llr_drange_fix,((double)errs[0])/((double)round_trials[0]));
  return(0);
}


