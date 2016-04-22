/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

/*! \file ulsim.c
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
#include "SIMULATION/TOOLS/defs.h"
#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/vars.h"

#include "SCHED/defs.h"
#include "SCHED/vars.h"
#include "LAYER2/MAC/vars.h"
#include "OCG_vars.h"

#include "PHY/TOOLS/lte_phy_scope.h"

extern unsigned short dftsizes[33];
extern short *ul_ref_sigs[30][2][33];

PHY_VARS_eNB *PHY_vars_eNB;
PHY_VARS_UE *PHY_vars_UE;

//#define MCS_COUNT 23//added for PHY abstraction

channel_desc_t *eNB2UE[NUMBER_OF_eNB_MAX][NUMBER_OF_UE_MAX];
channel_desc_t *UE2eNB[NUMBER_OF_UE_MAX][NUMBER_OF_eNB_MAX];
//Added for PHY abstraction
node_desc_t *enb_data[NUMBER_OF_eNB_MAX];
node_desc_t *ue_data[NUMBER_OF_UE_MAX];
//double sinr_bler_map[MCS_COUNT][2][16];

extern uint16_t beta_ack[16],beta_ri[16],beta_cqi[16];
//extern  char* namepointer_chMag ;

int xforms=0;
FD_lte_phy_scope_enb *form_enb;
char title[255];

/*the following parameters are used to control the processing times*/
double t_tx_max = -1000000000; /*!< \brief initial max process time for tx */
double t_rx_max = -1000000000; /*!< \brief initial max process time for rx */
double t_tx_min = 1000000000; /*!< \brief initial min process time for tx */
double t_rx_min = 1000000000; /*!< \brief initial min process time for tx */
int n_tx_dropped = 0; /*!< \brief initial max process time for tx */
int n_rx_dropped = 0; /*!< \brief initial max process time for rx */

int main(int argc, char **argv)
{

  char c;
  int i,j,aa,u;

  int aarx,aatx;
  double channelx,channely;
  double sigma2, sigma2_dB=10,SNR,SNR2,snr0=-2.0,snr1,SNRmeas,rate,saving_bler=0;
  double input_snr_step=.2,snr_int=30;
  double blerr;

  int **txdata;

  LTE_DL_FRAME_PARMS *frame_parms;
  double **s_re,**s_im,**r_re,**r_im;
  double forgetting_factor=0.0; //in [0,1] 0 means a new channel every time, 1 means keep the same channel
  double iqim=0.0;
  uint8_t extended_prefix_flag=0;
  int cqi_flag=0,cqi_error,cqi_errors,ack_errors,cqi_crc_falsepositives,cqi_crc_falsenegatives;
  int ch_realization;
  int eNB_id = 0;
  int chMod = 0 ;
  int UE_id = 0;
  unsigned char nb_rb=25,first_rb=0,mcs=0,round=0,bundling_flag=1;
  unsigned char l;

  unsigned char awgn_flag = 0 ;
  SCM_t channel_model=Rice1;


  unsigned char *input_buffer,harq_pid;
  unsigned short input_buffer_length;
  unsigned int ret;
  unsigned int coded_bits_per_codeword,nsymb;
  int subframe=3;
  unsigned int tx_lev=0,tx_lev_dB,trials,errs[4]= {0,0,0,0},round_trials[4]= {0,0,0,0};
  uint8_t transmission_mode=1,n_rx=1;

  FILE *bler_fd=NULL;
  char bler_fname[512];

  FILE *time_meas_fd=NULL;
  char time_meas_fname[256];

  FILE *input_fdUL=NULL,*trch_out_fdUL=NULL;
  //  unsigned char input_file=0;
  char input_val_str[50],input_val_str2[50];

  //  FILE *rx_frame_file;
  FILE *csv_fdUL=NULL;

  /*
  FILE *fperen=NULL;
  char fperen_name[512];

  FILE *fmageren=NULL;
  char fmageren_name[512];

  FILE *flogeren=NULL;
  char flogeren_name[512];
  */

  /* FILE *ftxlev;
     char ftxlev_name[512];
  */

  char csv_fname[512];
  int n_frames=5000;
  int n_ch_rlz = 1;
  int abstx = 0;
  int hold_channel=0;
  channel_desc_t *UE2eNB;

  uint8_t control_only_flag = 0;
  int delay = 0;
  double maxDoppler = 0.0;
  uint8_t srs_flag = 0;

  uint8_t N_RB_DL=25,osf=1;

  uint8_t cyclic_shift = 0;
  uint8_t cooperation_flag = 0; //0 no cooperation, 1 delay diversity, 2 Alamouti
  uint8_t beta_ACK=0,beta_RI=0,beta_CQI=2;
  uint8_t tdd_config=3,frame_type=FDD;

  uint8_t N0=30;
  double tx_gain=1.0;
  double cpu_freq_GHz;
  int avg_iter,iter_trials;

  uint32_t UL_alloc_pdu;
  int s,Kr,Kr_bytes;
  int dump_perf=0;
  int test_perf=0;
  int dump_table =0;

  double effective_rate=0.0;
  char channel_model_input[10];

  uint8_t max_turbo_iterations=4;
  uint8_t llr8_flag=0;
  int nb_rb_set = 0;
  int sf;

  int threequarter_fs=0;
  opp_enabled=1; // to enable the time meas

  cpu_freq_GHz = (double)get_cpu_freq_GHz();

  printf("Detected cpu_freq %f GHz\n",cpu_freq_GHz);


  logInit();

  while ((c = getopt (argc, argv, "hapZEbm:n:Y:X:x:s:w:e:q:d:D:O:c:r:i:f:y:c:oA:C:R:g:N:l:S:T:QB:PI:LF")) != -1) {
    switch (c) {
    case 'a':
      channel_model = AWGN;
      chMod = 1;
      break;

    case 'b':
      bundling_flag = 0;
      break;

    case 'd':
      delay = atoi(optarg);
      break;

    case 'D':
      maxDoppler = atoi(optarg);
      break;

    case 'm':
      mcs = atoi(optarg);
      break;

    case 'n':
      n_frames = atoi(optarg);
      break;

    case 'Y':
      n_ch_rlz = atoi(optarg);
      break;

    case 'X':
      abstx= atoi(optarg);
      break;

    case 'g':
      sprintf(channel_model_input,optarg,10);

      switch((char)*optarg) {
      case 'A':
        channel_model=SCM_A;
        chMod = 2;
        break;

      case 'B':
        channel_model=SCM_B;
        chMod = 3;
        break;

      case 'C':
        channel_model=SCM_C;
        chMod = 4;
        break;

      case 'D':
        channel_model=SCM_D;
        chMod = 5;
        break;

      case 'E':
        channel_model=EPA;
        chMod = 6;
        break;

      case 'F':
        channel_model=EVA;
        chMod = 7;
        break;

      case 'G':
        channel_model=ETU;
        chMod = 8;
        break;

      case 'H':
        channel_model=Rayleigh8;
        chMod = 9;
        break;

      case 'I':
        channel_model=Rayleigh1;
        chMod = 10;
        break;

      case 'J':
        channel_model=Rayleigh1_corr;
        chMod = 11;
        break;

      case 'K':
        channel_model=Rayleigh1_anticorr;
        chMod = 12;
        break;

      case 'L':
        channel_model=Rice8;
        chMod = 13;
        break;

      case 'M':
        channel_model=Rice1;
        chMod = 14;
        break;

      case 'N':
        channel_model=AWGN;
        chMod = 1;
        break;

      default:
        printf("Unsupported channel model!\n");
        exit(-1);
        break;
      }

      break;

    case 's':
      snr0 = atof(optarg);
      break;

    case 'w':
      snr_int = atof(optarg);
      break;

    case 'e':
      input_snr_step= atof(optarg);
      break;

    case 'x':
      transmission_mode=atoi(optarg);

      if ((transmission_mode!=1) &&
          (transmission_mode!=2)) {
        printf("Unsupported transmission mode %d\n",transmission_mode);
        exit(-1);
      }

      break;

    case 'y':
      n_rx = atoi(optarg);
      break;

    case 'S':
      subframe = atoi(optarg);
      break;

    case 'T':
      tdd_config=atoi(optarg);
      frame_type=TDD;
      break;

    case 'p':
      extended_prefix_flag=1;
      break;

    case 'r':
      nb_rb = atoi(optarg);
      nb_rb_set = 1;
      break;

    case 'f':
      first_rb = atoi(optarg);
      break;

    case 'c':
      cyclic_shift = atoi(optarg);
      break;

    case 'E':
      threequarter_fs=1;
      break;

    case 'N':
      N0 = atoi(optarg);
      break;

    case 'o':
      srs_flag = 1;
      break;

    case 'i':
      input_fdUL = fopen(optarg,"r");
      printf("Reading in %s (%p)\n",optarg,input_fdUL);

      if (input_fdUL == (FILE*)NULL) {
        printf("Unknown file %s\n",optarg);
        exit(-1);
      }

      //      input_file=1;
      break;

    case 'A':
      beta_ACK = atoi(optarg);

      if (beta_ACK>15) {
        printf("beta_ack must be in (0..15)\n");
        exit(-1);
      }

      break;

    case 'C':
      beta_CQI = atoi(optarg);

      if ((beta_CQI>15)||(beta_CQI<2)) {
        printf("beta_cqi must be in (2..15)\n");
        exit(-1);
      }

      break;

    case 'R':
      beta_RI = atoi(optarg);

      if ((beta_RI>15)||(beta_RI<2)) {
        printf("beta_ri must be in (0..13)\n");
        exit(-1);
      }

      break;

    case 'Q':
      cqi_flag=1;
      break;

    case 'B':
      N_RB_DL=atoi(optarg);
      break;

    case 'P':
      dump_perf=1;
      opp_enabled=1;
      break;

    case 'O':
      test_perf=atoi(optarg);
      //print_perf =1;
      break;

    case 'L':
      llr8_flag=1;
      break;

    case 'I':
      max_turbo_iterations=atoi(optarg);
      break;

    case 'F':
      xforms=1;
      break;

    case 'Z':
      dump_table = 1;
      break;

    case 'h':
    default:
      printf("%s -h(elp) -a(wgn on) -m mcs -n n_frames -s snr0 -t delay_spread -p (extended prefix on) -r nb_rb -f first_rb -c cyclic_shift -o (srs on) -g channel_model [A:M] Use 3GPP 25.814 SCM-A/B/C/D('A','B','C','D') or 36-101 EPA('E'), EVA ('F'),ETU('G') models (ignores delay spread and Ricean factor), Rayghleigh8 ('H'), Rayleigh1('I'), Rayleigh1_corr('J'), Rayleigh1_anticorr ('K'), Rice8('L'), Rice1('M'), -d Channel delay, -D maximum Doppler shift \n",
             argv[0]);
      exit(1);
      break;
    }
  }

  lte_param_init(1,
		 n_rx,
		 1,
		 extended_prefix_flag,
		 frame_type,
		 0,
		 tdd_config,
		 N_RB_DL,
		 threequarter_fs,
		 osf,
		 0);

  if (nb_rb_set == 0)
    nb_rb = PHY_vars_eNB->lte_frame_parms.N_RB_UL;

  printf("1 . rxdataF_comp[0] %p\n",PHY_vars_eNB->lte_eNB_pusch_vars[0]->rxdataF_comp[0][0]);
  printf("Setting mcs = %d\n",mcs);
  printf("n_frames = %d\n", n_frames);

  snr1 = snr0+snr_int;
  printf("SNR0 %f, SNR1 %f\n",snr0,snr1);

  /*
    txdataF    = (int **)malloc16(2*sizeof(int*));
    txdataF[0] = (int *)malloc16(FRAME_LENGTH_BYTES);
    txdataF[1] = (int *)malloc16(FRAME_LENGTH_BYTES);

    txdata    = (int **)malloc16(2*sizeof(int*));
    txdata[0] = (int *)malloc16(FRAME_LENGTH_BYTES);
    txdata[1] = (int *)malloc16(FRAME_LENGTH_BYTES);
  */

  frame_parms = &PHY_vars_eNB->lte_frame_parms;

  txdata = PHY_vars_UE->lte_ue_common_vars.txdata;


  s_re = malloc(2*sizeof(double*));
  s_im = malloc(2*sizeof(double*));
  r_re = malloc(2*sizeof(double*));
  r_im = malloc(2*sizeof(double*));
  //  r_re0 = malloc(2*sizeof(double*));
  //  r_im0 = malloc(2*sizeof(double*));

  nsymb = (PHY_vars_eNB->lte_frame_parms.Ncp == NORMAL) ? 14 : 12;


  sprintf(bler_fname,"ULbler_mcs%d_nrb%d_ChannelModel%d_nsim%d.csv",mcs,nb_rb,chMod,n_frames);
  bler_fd = fopen(bler_fname,"w");
  if (bler_fd==NULL) {
    fprintf(stderr,"Problem creating file %s\n",bler_fname);
    exit(-1);
  }

  fprintf(bler_fd,"#SNR;mcs;nb_rb;TBS;rate;errors[0];trials[0];errors[1];trials[1];errors[2];trials[2];errors[3];trials[3]\n");

  if (test_perf != 0) {
    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);
    printf("Hostname: %s\n", hostname);
    //char dirname[FILENAME_MAX];
    //sprintf(dirname, "%s//SIMU/USER/pre-ci-logs-%s", getenv("OPENAIR_TARGETS"),hostname);
    //mkdir(dirname, 0777);
    sprintf(time_meas_fname,"time_meas_prb%d_mcs%d_antrx%d_channel%s_tx%d.csv",
            N_RB_DL,mcs,n_rx,channel_model_input,transmission_mode);
    time_meas_fd = fopen(time_meas_fname,"w");
    if (time_meas_fd==NULL) {
      fprintf(stderr,"Cannot create file %s!\n",time_meas_fname);
      exit(-1);
    }
  }

  /*
  if(abstx) {
    sprintf(fperen_name,"ULchan_estims_F_mcs%d_rb%d_chanMod%d_nframes%d_chanReal%d.m",mcs,nb_rb,chMod,n_frames,n_ch_rlz);
    fperen = fopen(fperen_name,"a+");
    fprintf(fperen,"chest_f = [");
    fclose(fperen);

    sprintf(fmageren_name,"ChanMag_F_mcs%d_rb%d_chanMod%d_nframes%d_chanReal%d.m",mcs,nb_rb,chMod,n_frames,n_ch_rlz);
    fmageren = fopen(fmageren_name,"a+");
    fprintf(fmageren,"mag_f = [");
    fclose(fmageren);

    sprintf(flogeren_name,"Log2Max_mcs%d_rb%d_chanMod%d_nframes%d_chanReal%d.m",mcs,nb_rb,chMod,n_frames,n_ch_rlz);
    flogeren = fopen(flogeren_name,"a+");
    fprintf(flogeren,"mag_f = [");
    fclose(flogeren);
  }
  */

  /*
    sprintf(ftxlev_name,"txlevel_mcs%d_rb%d_chanMod%d_nframes%d_chanReal%d.m",mcs,nb_rb,chMod,n_frames,n_ch_rlz);
    ftxlev = fopen(ftxlev_name,"a+");
    fprintf(ftxlev,"txlev = [");
    fclose(ftexlv);
  */

  if(abstx) {
    // CSV file
    sprintf(csv_fname,"EULdataout_tx%d_mcs%d_nbrb%d_chan%d_nsimus%d_eren.m",transmission_mode,mcs,nb_rb,chMod,n_frames);
    csv_fdUL = fopen(csv_fname,"w");
    if (csv_fdUL == NULL) {
      fprintf(stderr,"Problem opening file %s\n",csv_fname);
      exit(-1);
    }
    fprintf(csv_fdUL,"data_all%d=[",mcs);
  }

  for (i=0; i<2; i++) {
    s_re[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    s_im[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    r_re[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    r_im[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    //    r_re0[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    //    bzero(r_re0[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    //    r_im0[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    //    bzero(r_im0[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
  }


  if (xforms==1) {
    fl_initialize (&argc, argv, NULL, 0, 0);
    form_enb = create_lte_phy_scope_enb();
    sprintf (title, "LTE PHY SCOPE eNB");
    fl_show_form (form_enb->lte_phy_scope_enb, FL_PLACE_HOTSPOT, FL_FULLBORDER, title);
  }

  PHY_vars_UE->lte_ue_pdcch_vars[0]->crnti = 14;

  PHY_vars_UE->lte_frame_parms.soundingrs_ul_config_common.srs_BandwidthConfig = 2;
  PHY_vars_UE->lte_frame_parms.soundingrs_ul_config_common.srs_SubframeConfig = 7;
  PHY_vars_UE->soundingrs_ul_config_dedicated[eNB_id].srs_Bandwidth = 0;
  PHY_vars_UE->soundingrs_ul_config_dedicated[eNB_id].transmissionComb = 0;
  PHY_vars_UE->soundingrs_ul_config_dedicated[eNB_id].freqDomainPosition = 0;

  PHY_vars_eNB->lte_frame_parms.soundingrs_ul_config_common.srs_BandwidthConfig = 2;
  PHY_vars_eNB->lte_frame_parms.soundingrs_ul_config_common.srs_SubframeConfig = 7;

  PHY_vars_eNB->soundingrs_ul_config_dedicated[UE_id].srs_ConfigIndex = 1;
  PHY_vars_eNB->soundingrs_ul_config_dedicated[UE_id].srs_Bandwidth = 0;
  PHY_vars_eNB->soundingrs_ul_config_dedicated[UE_id].transmissionComb = 0;
  PHY_vars_eNB->soundingrs_ul_config_dedicated[UE_id].freqDomainPosition = 0;
  PHY_vars_eNB->cooperation_flag = cooperation_flag;
  //  PHY_vars_eNB->eNB_UE_stats[0].SRS_parameters = PHY_vars_UE->SRS_parameters;

  PHY_vars_eNB->pusch_config_dedicated[UE_id].betaOffset_ACK_Index = beta_ACK;
  PHY_vars_eNB->pusch_config_dedicated[UE_id].betaOffset_RI_Index  = beta_RI;
  PHY_vars_eNB->pusch_config_dedicated[UE_id].betaOffset_CQI_Index = beta_CQI;
  PHY_vars_UE->pusch_config_dedicated[eNB_id].betaOffset_ACK_Index = beta_ACK;
  PHY_vars_UE->pusch_config_dedicated[eNB_id].betaOffset_RI_Index  = beta_RI;
  PHY_vars_UE->pusch_config_dedicated[eNB_id].betaOffset_CQI_Index = beta_CQI;

  PHY_vars_UE->ul_power_control_dedicated[eNB_id].deltaMCS_Enabled = 1;

  printf("PUSCH Beta : ACK %f, RI %f, CQI %f\n",(double)beta_ack[beta_ACK]/8,(double)beta_ri[beta_RI]/8,(double)beta_cqi[beta_CQI]/8);

  UE2eNB = new_channel_desc_scm(PHY_vars_eNB->lte_frame_parms.nb_antennas_tx,
                                PHY_vars_UE->lte_frame_parms.nb_antennas_rx,
                                channel_model,
				N_RB2sampling_rate(PHY_vars_eNB->lte_frame_parms.N_RB_UL),
				N_RB2channel_bandwidth(PHY_vars_eNB->lte_frame_parms.N_RB_UL),
                                forgetting_factor,
                                delay,
                                0);
  // set Doppler
  UE2eNB->max_Doppler = maxDoppler;

  // NN: N_RB_UL has to be defined in ulsim
  PHY_vars_eNB->ulsch_eNB[0] = new_eNB_ulsch(max_turbo_iterations,N_RB_DL,0);
  PHY_vars_UE->ulsch_ue[0]   = new_ue_ulsch(N_RB_DL,0);

  // Create transport channel structures for 2 transport blocks (MIMO)
  for (i=0; i<2; i++) {
    PHY_vars_eNB->dlsch_eNB[0][i] = new_eNB_dlsch(1,8,1827072,N_RB_DL,0);
    PHY_vars_UE->dlsch_ue[0][i]  = new_ue_dlsch(1,8,1827072,MAX_TURBO_ITERATIONS,N_RB_DL,0);

    if (!PHY_vars_eNB->dlsch_eNB[0][i]) {
      printf("Can't get eNB dlsch structures\n");
      exit(-1);
    }

    if (!PHY_vars_UE->dlsch_ue[0][i]) {
      printf("Can't get ue dlsch structures\n");
      exit(-1);
    }

    PHY_vars_eNB->dlsch_eNB[0][i]->rnti = 14;
    PHY_vars_UE->dlsch_ue[0][i]->rnti   = 14;

  } 


  switch (PHY_vars_eNB->lte_frame_parms.N_RB_UL) {
  case 6:
    break;

  case 25:
    if (PHY_vars_eNB->lte_frame_parms.frame_type == TDD) {
      ((DCI0_5MHz_TDD_1_6_t*)&UL_alloc_pdu)->type    = 0;
      ((DCI0_5MHz_TDD_1_6_t*)&UL_alloc_pdu)->rballoc = computeRIV(PHY_vars_eNB->lte_frame_parms.N_RB_UL,first_rb,nb_rb);// 12 RBs from position 8
      printf("nb_rb %d/%d, rballoc %d (dci %x)\n",nb_rb,PHY_vars_eNB->lte_frame_parms.N_RB_UL,((DCI0_5MHz_TDD_1_6_t*)&UL_alloc_pdu)->rballoc,*(uint32_t *)&UL_alloc_pdu);
      ((DCI0_5MHz_TDD_1_6_t*)&UL_alloc_pdu)->mcs     = mcs;
      ((DCI0_5MHz_TDD_1_6_t*)&UL_alloc_pdu)->ndi     = 1;
      ((DCI0_5MHz_TDD_1_6_t*)&UL_alloc_pdu)->TPC     = 0;
      ((DCI0_5MHz_TDD_1_6_t*)&UL_alloc_pdu)->cqi_req = cqi_flag&1;
      ((DCI0_5MHz_TDD_1_6_t*)&UL_alloc_pdu)->cshift  = 0;
      ((DCI0_5MHz_TDD_1_6_t*)&UL_alloc_pdu)->dai     = 1;
    } else {
      ((DCI0_5MHz_FDD_t*)&UL_alloc_pdu)->type    = 0;
      ((DCI0_5MHz_FDD_t*)&UL_alloc_pdu)->rballoc = computeRIV(PHY_vars_eNB->lte_frame_parms.N_RB_UL,first_rb,nb_rb);// 12 RBs from position 8
      printf("nb_rb %d/%d, rballoc %d (dci %x)\n",nb_rb,PHY_vars_eNB->lte_frame_parms.N_RB_UL,((DCI0_5MHz_FDD_t*)&UL_alloc_pdu)->rballoc,*(uint32_t *)&UL_alloc_pdu);
      ((DCI0_5MHz_FDD_t*)&UL_alloc_pdu)->mcs     = mcs;
      ((DCI0_5MHz_FDD_t*)&UL_alloc_pdu)->ndi     = 1;
      ((DCI0_5MHz_FDD_t*)&UL_alloc_pdu)->TPC     = 0;
      ((DCI0_5MHz_FDD_t*)&UL_alloc_pdu)->cqi_req = cqi_flag&1;
      ((DCI0_5MHz_FDD_t*)&UL_alloc_pdu)->cshift  = 0;
    }

    break;

  case 50:
    if (PHY_vars_eNB->lte_frame_parms.frame_type == TDD) {
      ((DCI0_10MHz_TDD_1_6_t*)&UL_alloc_pdu)->type    = 0;
      ((DCI0_10MHz_TDD_1_6_t*)&UL_alloc_pdu)->rballoc = computeRIV(PHY_vars_eNB->lte_frame_parms.N_RB_UL,first_rb,nb_rb);// 12 RBs from position 8
      printf("nb_rb %d/%d, rballoc %d (dci %x)\n",nb_rb,PHY_vars_eNB->lte_frame_parms.N_RB_UL,((DCI0_10MHz_TDD_1_6_t*)&UL_alloc_pdu)->rballoc,*(uint32_t *)&UL_alloc_pdu);
      ((DCI0_10MHz_TDD_1_6_t*)&UL_alloc_pdu)->mcs     = mcs;
      ((DCI0_10MHz_TDD_1_6_t*)&UL_alloc_pdu)->ndi     = 1;
      ((DCI0_10MHz_TDD_1_6_t*)&UL_alloc_pdu)->TPC     = 0;
      ((DCI0_10MHz_TDD_1_6_t*)&UL_alloc_pdu)->cqi_req = cqi_flag&1;
      ((DCI0_10MHz_TDD_1_6_t*)&UL_alloc_pdu)->cshift  = 0;
      ((DCI0_10MHz_TDD_1_6_t*)&UL_alloc_pdu)->dai     = 1;
    } else {
      ((DCI0_10MHz_FDD_t*)&UL_alloc_pdu)->type    = 0;
      ((DCI0_10MHz_FDD_t*)&UL_alloc_pdu)->rballoc = computeRIV(PHY_vars_eNB->lte_frame_parms.N_RB_UL,first_rb,nb_rb);// 12 RBs from position 8
      printf("nb_rb %d/%d, rballoc %d (dci %x)\n",nb_rb,PHY_vars_eNB->lte_frame_parms.N_RB_UL,((DCI0_10MHz_FDD_t*)&UL_alloc_pdu)->rballoc,*(uint32_t *)&UL_alloc_pdu);
      ((DCI0_10MHz_FDD_t*)&UL_alloc_pdu)->mcs     = mcs;
      ((DCI0_10MHz_FDD_t*)&UL_alloc_pdu)->ndi     = 1;
      ((DCI0_10MHz_FDD_t*)&UL_alloc_pdu)->TPC     = 0;
      ((DCI0_10MHz_FDD_t*)&UL_alloc_pdu)->cqi_req = cqi_flag&1;
      ((DCI0_10MHz_FDD_t*)&UL_alloc_pdu)->cshift  = 0;
    }

    break;

  case 100:
    if (PHY_vars_eNB->lte_frame_parms.frame_type == TDD) {
      ((DCI0_20MHz_TDD_1_6_t*)&UL_alloc_pdu)->type    = 0;
      ((DCI0_20MHz_TDD_1_6_t*)&UL_alloc_pdu)->rballoc = computeRIV(PHY_vars_eNB->lte_frame_parms.N_RB_UL,first_rb,nb_rb);// 12 RBs from position 8
      printf("nb_rb %d/%d, rballoc %d (dci %x)\n",nb_rb,PHY_vars_eNB->lte_frame_parms.N_RB_UL,((DCI0_20MHz_TDD_1_6_t*)&UL_alloc_pdu)->rballoc,*(uint32_t *)&UL_alloc_pdu);
      ((DCI0_20MHz_TDD_1_6_t*)&UL_alloc_pdu)->mcs     = mcs;
      ((DCI0_20MHz_TDD_1_6_t*)&UL_alloc_pdu)->ndi     = 1;
      ((DCI0_20MHz_TDD_1_6_t*)&UL_alloc_pdu)->TPC     = 0;
      ((DCI0_20MHz_TDD_1_6_t*)&UL_alloc_pdu)->cqi_req = cqi_flag&1;
      ((DCI0_20MHz_TDD_1_6_t*)&UL_alloc_pdu)->cshift  = 0;
      ((DCI0_20MHz_TDD_1_6_t*)&UL_alloc_pdu)->dai     = 1;
    } else {
      ((DCI0_20MHz_FDD_t*)&UL_alloc_pdu)->type    = 0;
      ((DCI0_20MHz_FDD_t*)&UL_alloc_pdu)->rballoc = computeRIV(PHY_vars_eNB->lte_frame_parms.N_RB_UL,first_rb,nb_rb);// 12 RBs from position 8
      printf("nb_rb %d/%d, rballoc %d (dci %x)\n",nb_rb,PHY_vars_eNB->lte_frame_parms.N_RB_UL,((DCI0_20MHz_FDD_t*)&UL_alloc_pdu)->rballoc,*(uint32_t *)&UL_alloc_pdu);
      ((DCI0_20MHz_FDD_t*)&UL_alloc_pdu)->mcs     = mcs;
      ((DCI0_20MHz_FDD_t*)&UL_alloc_pdu)->ndi     = 1;
      ((DCI0_20MHz_FDD_t*)&UL_alloc_pdu)->TPC     = 0;
      ((DCI0_20MHz_FDD_t*)&UL_alloc_pdu)->cqi_req = cqi_flag&1;
      ((DCI0_20MHz_FDD_t*)&UL_alloc_pdu)->cshift  = 0;
    }

    break;

  default:
    break;
  }


  PHY_vars_UE->PHY_measurements.rank[0] = 0;
  PHY_vars_UE->transmission_mode[0] = 2;
  PHY_vars_UE->pucch_config_dedicated[0].tdd_AckNackFeedbackMode = bundling_flag == 1 ? bundling : multiplexing;
  PHY_vars_eNB->transmission_mode[0] = 2;
  PHY_vars_eNB->pucch_config_dedicated[0].tdd_AckNackFeedbackMode = bundling_flag == 1 ? bundling : multiplexing;
  PHY_vars_UE->lte_frame_parms.pusch_config_common.ul_ReferenceSignalsPUSCH.groupHoppingEnabled = 1;
  PHY_vars_eNB->lte_frame_parms.pusch_config_common.ul_ReferenceSignalsPUSCH.groupHoppingEnabled = 1;
  PHY_vars_UE->lte_frame_parms.pusch_config_common.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled = 0;
  PHY_vars_eNB->lte_frame_parms.pusch_config_common.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled = 0;
  PHY_vars_UE->lte_frame_parms.pusch_config_common.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH = 0;
  PHY_vars_eNB->lte_frame_parms.pusch_config_common.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH = 0;
  PHY_vars_UE->frame_tx=1;

  for (sf=0; sf<10; sf++) {
    PHY_vars_eNB->proc[sf].frame_tx=1;
    PHY_vars_eNB->proc[sf].subframe_tx=sf;
    PHY_vars_eNB->proc[sf].frame_rx=1;
    PHY_vars_eNB->proc[sf].subframe_rx=sf;
  }

  printf("Init UL hopping UE\n");
  init_ul_hopping(&PHY_vars_UE->lte_frame_parms);
  printf("Init UL hopping eNB\n");
  init_ul_hopping(&PHY_vars_eNB->lte_frame_parms);

  PHY_vars_eNB->proc[subframe].frame_rx = PHY_vars_UE->frame_tx;

  if (ul_subframe2pdcch_alloc_subframe(&PHY_vars_eNB->lte_frame_parms,subframe) > subframe) // allocation was in previous frame
    PHY_vars_eNB->proc[ul_subframe2pdcch_alloc_subframe(&PHY_vars_eNB->lte_frame_parms,subframe)].frame_tx = (PHY_vars_UE->frame_tx-1)&1023;

  PHY_vars_UE->dlsch_ue[0][0]->harq_ack[ul_subframe2pdcch_alloc_subframe(&PHY_vars_eNB->lte_frame_parms,subframe)].send_harq_status = 1;


  //  printf("UE frame %d, eNB frame %d (eNB frame_tx %d)\n",PHY_vars_UE->frame,PHY_vars_eNB->proc[subframe].frame_rx,PHY_vars_eNB->proc[ul_subframe2pdcch_alloc_subframe(&PHY_vars_eNB->lte_frame_parms,subframe)].frame_tx);
  PHY_vars_UE->frame_tx = (PHY_vars_UE->frame_tx-1)&1023;

  generate_ue_ulsch_params_from_dci((void *)&UL_alloc_pdu,
                                    14,
                                    ul_subframe2pdcch_alloc_subframe(&PHY_vars_UE->lte_frame_parms,subframe),
                                    format0,
                                    PHY_vars_UE,
                                    SI_RNTI,
                                    0,
                                    P_RNTI,
                                    CBA_RNTI,
                                    0,
                                    srs_flag);

  //  printf("RIV %d\n",UL_alloc_pdu.rballoc);

  generate_eNB_ulsch_params_from_dci((void *)&UL_alloc_pdu,
                                     14,
                                     ul_subframe2pdcch_alloc_subframe(&PHY_vars_eNB->lte_frame_parms,subframe),
                                     format0,
                                     0,
                                     PHY_vars_eNB,
                                     SI_RNTI,
                                     0,
                                     P_RNTI,
                                     CBA_RNTI,
                                     srs_flag);

  coded_bits_per_codeword = nb_rb * (12 * get_Qm_ul(mcs)) * nsymb;

  if (cqi_flag == 1) coded_bits_per_codeword-=PHY_vars_UE->ulsch_ue[0]->O;

  rate = (double)dlsch_tbs25[get_I_TBS(mcs)][nb_rb-1]/(coded_bits_per_codeword);

  printf("Rate = %f (mod %d), coded bits %d\n",rate,get_Qm_ul(mcs),coded_bits_per_codeword);


  PHY_vars_UE->frame_tx = (PHY_vars_UE->frame_tx+1)&1023;


  for (ch_realization=0; ch_realization<n_ch_rlz; ch_realization++) {

    /*
      if(abstx){
      int ulchestim_f[300*12];
      int ulchestim_t[2*(frame_parms->ofdm_symbol_size)];
      }
    */

    if(abstx) {
      printf("**********************Channel Realization Index = %d **************************\n", ch_realization);
      saving_bler=1;
    }


    //    if ((subframe>5) || (subframe < 4))
    //      PHY_vars_UE->frame++;

    for (SNR=snr0; SNR<snr1; SNR+=input_snr_step) {
      errs[0]=0;
      errs[1]=0;
      errs[2]=0;
      errs[3]=0;
      round_trials[0] = 0;
      round_trials[1] = 0;
      round_trials[2] = 0;
      round_trials[3] = 0;
      cqi_errors=0;
      ack_errors=0;
      cqi_crc_falsepositives=0;
      cqi_crc_falsenegatives=0;
      round=0;

      //randominit(0);


      harq_pid = subframe2harq_pid(&PHY_vars_UE->lte_frame_parms,PHY_vars_UE->frame_tx,subframe);
      input_buffer_length = PHY_vars_UE->ulsch_ue[0]->harq_processes[harq_pid]->TBS/8;
      input_buffer = (unsigned char *)malloc(input_buffer_length+4);
      //      printf("UL frame %d/subframe %d, harq_pid %d\n",PHY_vars_UE->frame,subframe,harq_pid);
      if (input_fdUL == NULL) {


        if (n_frames == 1) {
          trch_out_fdUL= fopen("ulsch_trchUL.txt","w");

          for (i=0; i<input_buffer_length; i++) {
            input_buffer[i] = taus()&0xff;

            for (j=0; j<8; j++)
              fprintf(trch_out_fdUL,"%d\n",(input_buffer[i]>>(7-j))&1);
          }

          fclose(trch_out_fdUL);
        } else {
          for (i=0; i<input_buffer_length; i++)
            input_buffer[i] = taus()&0xff;
        }
      } else {
        n_frames=1;
        i=0;

        while (!feof(input_fdUL)) {
          ret=fscanf(input_fdUL,"%s %s",input_val_str,input_val_str2);//&input_val1,&input_val2);

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
        //      write_output("txsig0UL.m","txs0", txdata[0],2*frame_parms->samples_per_tti,1,1);
        //    write_output("txsig1.m","txs1", txdata[1],FRAME_LENGTH_COMPLEX_SAMPLES,1,1);
        tx_lev = signal_energy(&txdata[0][0],
                               OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES);
        tx_lev_dB = (unsigned int) dB_fixed(tx_lev);

      }

      avg_iter = 0;
      iter_trials=0;
      reset_meas(&PHY_vars_UE->phy_proc_tx);
      reset_meas(&PHY_vars_UE->ofdm_mod_stats);
      reset_meas(&PHY_vars_UE->ulsch_modulation_stats);
      reset_meas(&PHY_vars_UE->ulsch_encoding_stats);
      reset_meas(&PHY_vars_UE->ulsch_interleaving_stats);
      reset_meas(&PHY_vars_UE->ulsch_rate_matching_stats);
      reset_meas(&PHY_vars_UE->ulsch_turbo_encoding_stats);
      reset_meas(&PHY_vars_UE->ulsch_segmentation_stats);
      reset_meas(&PHY_vars_UE->ulsch_multiplexing_stats);

      reset_meas(&PHY_vars_eNB->phy_proc_rx);
      reset_meas(&PHY_vars_eNB->ofdm_demod_stats);
      reset_meas(&PHY_vars_eNB->ulsch_channel_estimation_stats);
      reset_meas(&PHY_vars_eNB->ulsch_freq_offset_estimation_stats);
      reset_meas(&PHY_vars_eNB->rx_dft_stats);
      reset_meas(&PHY_vars_eNB->ulsch_decoding_stats);
      reset_meas(&PHY_vars_eNB->ulsch_turbo_decoding_stats);
      reset_meas(&PHY_vars_eNB->ulsch_deinterleaving_stats);
      reset_meas(&PHY_vars_eNB->ulsch_demultiplexing_stats);
      reset_meas(&PHY_vars_eNB->ulsch_rate_unmatching_stats);
      reset_meas(&PHY_vars_eNB->ulsch_tc_init_stats);
      reset_meas(&PHY_vars_eNB->ulsch_tc_alpha_stats);
      reset_meas(&PHY_vars_eNB->ulsch_tc_beta_stats);
      reset_meas(&PHY_vars_eNB->ulsch_tc_gamma_stats);
      reset_meas(&PHY_vars_eNB->ulsch_tc_ext_stats);
      reset_meas(&PHY_vars_eNB->ulsch_tc_intl1_stats);
      reset_meas(&PHY_vars_eNB->ulsch_tc_intl2_stats);

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
        //      printf("*");
        //        PHY_vars_UE->frame++;
        //        PHY_vars_eNB->frame++;

        fflush(stdout);
        round=0;

        while (round < 4) {
          PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->round=round;
          PHY_vars_UE->ulsch_ue[0]->harq_processes[harq_pid]->round=round;
          //  printf("Trial %d : Round %d ",trials,round);
          round_trials[round]++;

          if (round == 0) {
            //PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->Ndi = 1;
            PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->rvidx = round>>1;
            //PHY_vars_UE->ulsch_ue[0]->harq_processes[harq_pid]->Ndi = 1;
            PHY_vars_UE->ulsch_ue[0]->harq_processes[harq_pid]->rvidx = round>>1;
          } else {
            //PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->Ndi = 0;
            PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->rvidx = round>>1;
            //PHY_vars_UE->ulsch_ue[0]->harq_processes[harq_pid]->Ndi = 0;
            PHY_vars_UE->ulsch_ue[0]->harq_processes[harq_pid]->rvidx = round>>1;
          }


          /////////////////////
          if (abstx) {
            if (trials==0 && round==0 && SNR==snr0) { //generate a new channel
              hold_channel = 0;
              flagMag=0;
            } else {
              hold_channel = 1;
              flagMag = 1;
            }
          } else {
            hold_channel = 0;
            flagMag=1;
          }

          ///////////////////////////////////////

          if (input_fdUL == NULL) {

            start_meas(&PHY_vars_UE->phy_proc_tx);

#ifdef OFDMA_ULSCH

            if (srs_flag)
              generate_srs_tx(PHY_vars_UE,0,AMP,subframe);

            generate_drs_pusch(PHY_vars_UE,0,AMP,subframe,first_rb,nb_rb,0);

#else

            if (srs_flag)
              generate_srs_tx(PHY_vars_UE,0,AMP,subframe);

            generate_drs_pusch(PHY_vars_UE,0,
                               AMP,subframe,
                               PHY_vars_UE->ulsch_ue[0]->harq_processes[harq_pid]->first_rb,
                               PHY_vars_UE->ulsch_ue[0]->harq_processes[harq_pid]->nb_rb,
                               0);
#endif

            if ((cqi_flag == 1) && (n_frames == 1) ) {
              printf("CQI information (O %d) %d %d\n",PHY_vars_UE->ulsch_ue[0]->O,
                     PHY_vars_UE->ulsch_ue[0]->o[0],PHY_vars_UE->ulsch_ue[0]->o[1]);
              print_CQI(PHY_vars_UE->ulsch_ue[0]->o,PHY_vars_UE->ulsch_ue[0]->uci_format,PHY_vars_UE->lte_frame_parms.N_RB_DL,0);
            }

            PHY_vars_UE->ulsch_ue[0]->o_ACK[0] = taus()&1;

            start_meas(&PHY_vars_UE->ulsch_encoding_stats);

            if (ulsch_encoding(input_buffer,
                               PHY_vars_UE,
                               harq_pid,
                               eNB_id,
                               2, // transmission mode
                               control_only_flag,
                               1// Nbundled
                              )==-1) {
              printf("ulsim.c Problem with ulsch_encoding\n");
              exit(-1);
            }

            stop_meas(&PHY_vars_UE->ulsch_encoding_stats);

            start_meas(&PHY_vars_UE->ulsch_modulation_stats);
#ifdef OFDMA_ULSCH
            ulsch_modulation(PHY_vars_UE->lte_ue_common_vars.txdataF,AMP,
                             PHY_vars_UE->frame_tx,subframe,&PHY_vars_UE->lte_frame_parms,PHY_vars_UE->ulsch_ue[0]);
#else
            //    printf("Generating PUSCH in subframe %d with amp %d, nb_rb %d\n",subframe,AMP,nb_rb);
            ulsch_modulation(PHY_vars_UE->lte_ue_common_vars.txdataF,AMP,
                             PHY_vars_UE->frame_tx,subframe,&PHY_vars_UE->lte_frame_parms,
                             PHY_vars_UE->ulsch_ue[0]);
#endif
            stop_meas(&PHY_vars_UE->ulsch_modulation_stats);

            if (n_frames==1) {
              write_output("txsigF0UL.m","txsF0", &PHY_vars_UE->lte_ue_common_vars.txdataF[0][PHY_vars_eNB->lte_frame_parms.ofdm_symbol_size*nsymb*subframe],PHY_vars_eNB->lte_frame_parms.ofdm_symbol_size*nsymb,1,
                           1);
              //write_output("txsigF1.m","txsF1", PHY_vars_UE->lte_ue_common_vars.txdataF[0],FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX,1,1);
            }

            tx_lev=0;
            start_meas(&PHY_vars_UE->ofdm_mod_stats);

            for (aa=0; aa<1; aa++) {
              if (frame_parms->Ncp == EXTENDED)
                PHY_ofdm_mod(&PHY_vars_UE->lte_ue_common_vars.txdataF[aa][subframe*nsymb*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES_NO_PREFIX],        // input
                             &txdata[aa][PHY_vars_eNB->lte_frame_parms.samples_per_tti*subframe],         // output
                             PHY_vars_UE->lte_frame_parms.ofdm_symbol_size,
                             nsymb,                 // number of symbols
                             PHY_vars_UE->lte_frame_parms.nb_prefix_samples,               // number of prefix samples
                             CYCLIC_PREFIX);
              else
                normal_prefix_mod(&PHY_vars_UE->lte_ue_common_vars.txdataF[aa][subframe*nsymb*OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES_NO_PREFIX],
                                  &txdata[aa][PHY_vars_eNB->lte_frame_parms.samples_per_tti*subframe],
                                  nsymb,
                                  frame_parms);

#ifndef OFDMA_ULSCH
              apply_7_5_kHz(PHY_vars_UE,PHY_vars_UE->lte_ue_common_vars.txdata[aa],subframe<<1);
              apply_7_5_kHz(PHY_vars_UE,PHY_vars_UE->lte_ue_common_vars.txdata[aa],1+(subframe<<1));
#endif

              stop_meas(&PHY_vars_UE->ofdm_mod_stats);
              stop_meas(&PHY_vars_UE->phy_proc_tx);
              tx_lev += signal_energy(&txdata[aa][PHY_vars_eNB->lte_frame_parms.samples_per_tti*subframe],
                                      PHY_vars_eNB->lte_frame_parms.samples_per_tti);

            }
          }  // input_fd == NULL


          tx_lev_dB = (unsigned int) dB_fixed_times10(tx_lev);

          if (n_frames==1) {
            write_output("txsig0UL.m","txs0", &txdata[0][PHY_vars_eNB->lte_frame_parms.samples_per_tti*subframe],2*frame_parms->samples_per_tti,1,1);
            //        write_output("txsig1UL.m","txs1", &txdata[1][PHY_vars_eNB->lte_frame_parms.samples_per_tti*subframe],2*frame_parms->samples_per_tti,1,1);
          }

          //AWGN
          //Set target wideband RX noise level to N0
          sigma2_dB = N0;//10*log10((double)tx_lev)  +10*log10(PHY_vars_UE->lte_frame_parms.ofdm_symbol_size/(PHY_vars_UE->lte_frame_parms.N_RB_DL*12)) - SNR;
          sigma2 = pow(10,sigma2_dB/10);

          // compute tx_gain to achieve target SNR (per resource element!)
          tx_gain = sqrt(pow(10.0,.1*(N0+SNR))*(nb_rb*12/(double)PHY_vars_UE->lte_frame_parms.ofdm_symbol_size)/(double)tx_lev);

          if (n_frames==1)
            printf("tx_lev = %d (%d.%d dB,%f), gain %f\n",tx_lev,tx_lev_dB/10,tx_lev_dB,10*log10((double)tx_lev),10*log10(tx_gain));


          // fill measurement symbol (19) with noise
          for (i=0; i<OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES; i++) {
            for (aa=0; aa<PHY_vars_eNB->lte_frame_parms.nb_antennas_rx; aa++) {

              ((short*) &PHY_vars_eNB->lte_eNB_common_vars.rxdata[0][aa][(frame_parms->samples_per_tti<<1) -frame_parms->ofdm_symbol_size])[2*i] = (short) ((sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
              ((short*) &PHY_vars_eNB->lte_eNB_common_vars.rxdata[0][aa][(frame_parms->samples_per_tti<<1) -frame_parms->ofdm_symbol_size])[2*i+1] = (short) ((sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
            }
          }

          // multipath channel

          for (i=0; i<PHY_vars_eNB->lte_frame_parms.samples_per_tti; i++) {
            for (aa=0; aa<1; aa++) {
              s_re[aa][i] = ((double)(((short *)&txdata[aa][PHY_vars_eNB->lte_frame_parms.samples_per_tti*subframe]))[(i<<1)]);
              s_im[aa][i] = ((double)(((short *)&txdata[aa][PHY_vars_eNB->lte_frame_parms.samples_per_tti*subframe]))[(i<<1)+1]);
            }
          }

          if (awgn_flag == 0) {
            if (UE2eNB->max_Doppler == 0) {
              multipath_channel(UE2eNB,s_re,s_im,r_re,r_im,
                                PHY_vars_eNB->lte_frame_parms.samples_per_tti,hold_channel);
            } else {
              multipath_tv_channel(UE2eNB,s_re,s_im,r_re,r_im,
                                   2*PHY_vars_eNB->lte_frame_parms.samples_per_tti,hold_channel);
            }
          }

          if(abstx) {
            if(saving_bler==0)
              if (trials==0 && round==0) {
                // calculate freq domain representation to compute SINR
                freq_channel(UE2eNB, N_RB_DL,12*N_RB_DL + 1);

                // snr=pow(10.0,.1*SNR);
                fprintf(csv_fdUL,"%f,%d,%d,%f,%f,%f,",SNR,tx_lev,tx_lev_dB,sigma2_dB,tx_gain,SNR2);

                //fprintf(csv_fdUL,"%f,",SNR);
                for (u=0; u<12*nb_rb; u++) {
                  for (aarx=0; aarx<UE2eNB->nb_rx; aarx++) {
                    for (aatx=0; aatx<UE2eNB->nb_tx; aatx++) {
                      // abs_channel = (eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][u].x*eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][u].x + eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][u].y*eNB2UE->chF[aarx+(aatx*eNB2UE->nb_rx)][u].y);
                      channelx = UE2eNB->chF[aarx+(aatx*UE2eNB->nb_rx)][u].x;
                      channely = UE2eNB->chF[aarx+(aatx*UE2eNB->nb_rx)][u].y;
                      // if(transmission_mode==5){
                      fprintf(csv_fdUL,"%e+i*(%e),",channelx,channely);
                      // }
                      // else{
                      //  pilot_sinr = 10*log10(snr*abs_channel);
                      //  fprintf(csv_fd,"%e,",pilot_sinr);
                      // }
                    }
                  }
                }
              }
          }

          if (n_frames==1)
            printf("Sigma2 %f (sigma2_dB %f), tx_gain %f (%f dB)\n",sigma2,sigma2_dB,tx_gain,20*log10(tx_gain));

          for (i=0; i<PHY_vars_eNB->lte_frame_parms.samples_per_tti; i++) {
            for (aa=0; aa<PHY_vars_eNB->lte_frame_parms.nb_antennas_rx; aa++) {
              ((short*) &PHY_vars_eNB->lte_eNB_common_vars.rxdata[0][aa][PHY_vars_eNB->lte_frame_parms.samples_per_tti*subframe])[2*i] = (short) ((tx_gain*r_re[aa][i]) + sqrt(sigma2/2)*gaussdouble(0.0,1.0));
              ((short*) &PHY_vars_eNB->lte_eNB_common_vars.rxdata[0][aa][PHY_vars_eNB->lte_frame_parms.samples_per_tti*subframe])[2*i+1] = (short) ((tx_gain*r_im[aa][i]) + (iqim*tx_gain*r_re[aa][i]) + sqrt(
                    sigma2/2)*gaussdouble(0.0,1.0));
            }
          }

          if (n_frames==1) {
            printf("rx_level Null symbol %f\n",10*log10((double)signal_energy((int*)
                   &PHY_vars_eNB->lte_eNB_common_vars.rxdata[0][0][(PHY_vars_eNB->lte_frame_parms.samples_per_tti<<1) -PHY_vars_eNB->lte_frame_parms.ofdm_symbol_size],OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES/2)));
            printf("rx_level data symbol %f\n",10*log10(signal_energy((int*)&PHY_vars_eNB->lte_eNB_common_vars.rxdata[0][0][160+(PHY_vars_eNB->lte_frame_parms.samples_per_tti*subframe)],
                   OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES/2)));
          }

          SNRmeas = 10*log10(((double)signal_energy((int*)&PHY_vars_eNB->lte_eNB_common_vars.rxdata[0][0][160+(PHY_vars_eNB->lte_frame_parms.samples_per_tti*subframe)],
                              OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES/2))/((double)signal_energy((int*)
                                  &PHY_vars_eNB->lte_eNB_common_vars.rxdata[0][0][(PHY_vars_eNB->lte_frame_parms.samples_per_tti<<1) -PHY_vars_eNB->lte_frame_parms.ofdm_symbol_size],
                                  OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES/2)) - 1)+10*log10(PHY_vars_eNB->lte_frame_parms.N_RB_UL/nb_rb);

          if (n_frames==1) {
            printf("SNRmeas %f\n",SNRmeas);

            //    write_output("rxsig0UL.m","rxs0", &PHY_vars_eNB->lte_eNB_common_vars.rxdata[0][0][PHY_vars_eNB->lte_frame_parms.samples_per_tti*subframe],PHY_vars_eNB->lte_frame_parms.samples_per_tti,1,1);
            //write_output("rxsig1UL.m","rxs1", &PHY_vars_eNB->lte_eNB_common_vars.rxdata[0][0][PHY_vars_eNB->lte_frame_parms.samples_per_tti*subframe],PHY_vars_eNB->lte_frame_parms.samples_per_tti,1,1);
          }

#ifndef OFDMA_ULSCH
          remove_7_5_kHz(PHY_vars_eNB,subframe<<1);
          remove_7_5_kHz(PHY_vars_eNB,1+(subframe<<1));
          //  write_output("rxsig0_75.m","rxs0_75", &PHY_vars_eNB->lte_eNB_common_vars.rxdata[0][0][PHY_vars_eNB->lte_frame_parms.samples_per_tti*subframe],PHY_vars_eNB->lte_frame_parms.samples_per_tti,1,1);
          //  write_output("rxsig1_75.m","rxs1_75", &PHY_vars_eNB->lte_eNB_common_vars.rxdata[0][0][PHY_vars_eNB->lte_frame_parms.samples_per_tti*subframe],PHY_vars_eNB->lte_frame_parms.samples_per_tti,1,1);

#endif

          start_meas(&PHY_vars_eNB->phy_proc_rx);
          start_meas(&PHY_vars_eNB->ofdm_demod_stats);
          lte_eNB_I0_measurements(PHY_vars_eNB,
				  subframe,
                                  0,
                                  1);

          for (l=subframe*PHY_vars_UE->lte_frame_parms.symbols_per_tti; l<((1+subframe)*PHY_vars_UE->lte_frame_parms.symbols_per_tti); l++) {

            slot_fep_ul(&PHY_vars_eNB->lte_frame_parms,
                        &PHY_vars_eNB->lte_eNB_common_vars,
                        l%(PHY_vars_eNB->lte_frame_parms.symbols_per_tti/2),
                        l/(PHY_vars_eNB->lte_frame_parms.symbols_per_tti/2),
                        0,
                        0);
          }

          stop_meas(&PHY_vars_eNB->ofdm_demod_stats);

          PHY_vars_eNB->ulsch_eNB[0]->cyclicShift = cyclic_shift;// cyclic shift for DMRS

	  /*
          if(abstx) {
            namepointer_log2 = &flogeren_name;
            namepointer_chMag = &fmageren_name;
            //namepointer_txlev = &ftxlev;
          }
	  */

          start_meas(&PHY_vars_eNB->ulsch_demodulation_stats);
          rx_ulsch(PHY_vars_eNB,
                   subframe,
                   0,  // this is the effective sector id
                   0,  // this is the UE_id
                   PHY_vars_eNB->ulsch_eNB,
                   cooperation_flag);
          stop_meas(&PHY_vars_eNB->ulsch_demodulation_stats);

	  /*
          if(abstx) {
            namepointer_chMag = NULL;

            if(trials==0 && round==0 && SNR==snr0) {
              char* namepointer ;
              namepointer = &fperen_name;
              write_output(namepointer, "xxx" ,PHY_vars_eNB->lte_eNB_pusch_vars[0]->drs_ch_estimates[0][0],300,1,10);
              namepointer = NULL ;
              // flagMag = 1;
            }
          }
	  */

          start_meas(&PHY_vars_eNB->ulsch_decoding_stats);

          ret= ulsch_decoding(PHY_vars_eNB,
                              0, // UE_id
                              subframe,
                              control_only_flag,
                              1,  // Nbundled
                              llr8_flag);
          stop_meas(&PHY_vars_eNB->ulsch_decoding_stats);
          stop_meas(&PHY_vars_eNB->phy_proc_rx);

          if (cqi_flag > 0) {
            cqi_error = 0;

            if (PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->Or1 < 32) {
              for (i=2; i<4; i++) {
                //                printf("cqi %d : %d (%d)\n",i,PHY_vars_eNB->ulsch_eNB[0]->o[i],PHY_vars_UE->ulsch_ue[0]->o[i]);
                if (PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->o[i] != PHY_vars_UE->ulsch_ue[0]->o[i])
                  cqi_error = 1;
              }
            } else {

            }

            if (cqi_error == 1) {
              cqi_errors++;

              if (PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->cqi_crc_status == 1)
                cqi_crc_falsepositives++;
            } else {
              if (PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->cqi_crc_status == 0)
                cqi_crc_falsenegatives++;
            }
          }

          if (PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->o_ACK[0] != PHY_vars_UE->ulsch_ue[0]->o_ACK[0])
            ack_errors++;

          //    printf("ulsch_coding: O[%d] %d\n",i,o_flip[i]);


          if (ret <= PHY_vars_eNB->ulsch_eNB[0]->max_turbo_iterations) {

            avg_iter += ret;
            iter_trials++;

            if (n_frames==1) {
              printf("No ULSCH errors found, o_ACK[0]= %d, cqi_crc_status=%d\n",PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->o_ACK[0],PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->cqi_crc_status);

              if (PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->cqi_crc_status==1)
                print_CQI(PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->o,
                          PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->uci_format,0,PHY_vars_eNB->lte_frame_parms.N_RB_DL);

              dump_ulsch(PHY_vars_eNB,subframe,0);
              exit(-1);
            }

            round=5;
          } else {
            avg_iter += ret-1;
            iter_trials++;

            errs[round]++;

            if (n_frames==1) {
              printf("ULSCH errors found o_ACK[0]= %d\n",PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->o_ACK[0]);

              for (s=0; s<PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->C; s++) {
                if (s<PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->Cminus)
                  Kr = PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->Kminus;
                else
                  Kr = PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->Kplus;

                Kr_bytes = Kr>>3;

                printf("Decoded_output (Segment %d):\n",s);

                for (i=0; i<Kr_bytes; i++)
                  printf("%d : %x (%x)\n",i,PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->c[s][i],
                         PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->c[s][i]^PHY_vars_UE->ulsch_ue[0]->harq_processes[harq_pid]->c[s][i]);
              }

              dump_ulsch(PHY_vars_eNB,subframe,0);
              exit(-1);
            }

            //      printf("round %d errors %d/%d\n",round,errs[round],trials);
            round++;

            if (n_frames==1) {
              printf("ULSCH in error in round %d\n",round);
            }
          }  // ulsch error
        } // round

        //      printf("\n");
        if ((errs[0]>=100) && (trials>(n_frames/2)))
          break;

	if (xforms==1)
	  phy_scope_eNB(form_enb,PHY_vars_eNB,0);

        /*calculate the total processing time for each packet, get the max, min, and number of packets that exceed t>3000us*/

        double t_tx = (double)PHY_vars_UE->phy_proc_tx.p_time/cpu_freq_GHz/1000.0;
        double t_tx_ifft = (double)PHY_vars_UE->ofdm_mod_stats.p_time/cpu_freq_GHz/1000.0;
        double t_tx_mod = (double)PHY_vars_UE->ulsch_modulation_stats.p_time/cpu_freq_GHz/1000.0;
        double t_tx_enc = (double)PHY_vars_UE->ulsch_encoding_stats.p_time/cpu_freq_GHz/1000.0;


        double t_rx = (double)PHY_vars_eNB->phy_proc_rx.p_time/cpu_freq_GHz/1000.0;
        double t_rx_fft = (double)PHY_vars_eNB->ofdm_demod_stats.p_time/cpu_freq_GHz/1000.0;
        double t_rx_demod = (double)PHY_vars_eNB->ulsch_demodulation_stats.p_time/cpu_freq_GHz/1000.0;
        double t_rx_dec = (double)PHY_vars_eNB->ulsch_decoding_stats.p_time/cpu_freq_GHz/1000.0;

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
        int n;
        set_component_filelog(USIM); // file located in /tmp/usim.txt
        LOG_F(USIM,"The transmitter raw data: \n");

        for (n=0; n< time_vector_tx.size; n++) {
          //   printf("%f ", table_tx[n]);
          LOG_F(USIM,"%f ", table_tx[n]);
        }

        LOG_F(USIM,"\n");
        LOG_F(USIM,"The receiver raw data: \n");

        for (n=0; n< time_vector_rx.size; n++) {
          // printf("%f ", table_rx[n]);
          LOG_F(USIM,"%f ", table_rx[n]);
        }

        LOG_F(USIM,"\n");
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

      printf("\n**********rb: %d ***mcs : %d  *********SNR = %f dB (%f): TX %d dB (gain %f dB), N0W %f dB, I0 %d dB, delta_IF %d [ (%d,%d) dB / (%d,%d) dB ]**************************\n",
             nb_rb,mcs,SNR,SNR2,
             tx_lev_dB,
             20*log10(tx_gain),
             (double)N0,
             PHY_vars_eNB->PHY_measurements_eNB[0].n0_power_tot_dB,
             get_hundred_times_delta_IF(PHY_vars_UE,eNB_id,harq_pid) ,
             dB_fixed(PHY_vars_eNB->lte_eNB_pusch_vars[0]->ulsch_power[0]),
             dB_fixed(PHY_vars_eNB->lte_eNB_pusch_vars[0]->ulsch_power[1]),
             PHY_vars_eNB->PHY_measurements_eNB->n0_power_dB[0],
             PHY_vars_eNB->PHY_measurements_eNB->n0_power_dB[1]);

      effective_rate = ((double)(round_trials[0])/((double)round_trials[0] + round_trials[1] + round_trials[2] + round_trials[3]));

      printf("Errors (%d/%d %d/%d %d/%d %d/%d), Pe = (%e,%e,%e,%e) => effective rate %f (%3.1f%%,%f,%f), normalized delay %f (%f)\n",
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
             rate*effective_rate,
             100*effective_rate,
             rate,
             rate*get_Qm_ul(mcs),
             (1.0*(round_trials[0]-errs[0])+2.0*(round_trials[1]-errs[1])+3.0*(round_trials[2]-errs[2])+4.0*(round_trials[3]-errs[3]))/((double)round_trials[0])/
             (double)PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->TBS,
             (1.0*(round_trials[0]-errs[0])+2.0*(round_trials[1]-errs[1])+3.0*(round_trials[2]-errs[2])+4.0*(round_trials[3]-errs[3]))/((double)round_trials[0]));

      if (cqi_flag >0) {
        printf("CQI errors %d/%d,false positives %d/%d, CQI false negatives %d/%d\n",
               cqi_errors,round_trials[0]+round_trials[1]+round_trials[2]+round_trials[3],
               cqi_crc_falsepositives,round_trials[0]+round_trials[1]+round_trials[2]+round_trials[3],
               cqi_crc_falsenegatives,round_trials[0]+round_trials[1]+round_trials[2]+round_trials[3]);
      }

      if (PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->o_ACK[0] > 0)
        printf("ACK/NAK errors %d/%d\n",ack_errors,round_trials[0]+round_trials[1]+round_trials[2]+round_trials[3]);


      fprintf(bler_fd,"%f;%d;%d;%d;%f;%d;%d;%d;%d;%d;%d;%d;%d\n",
              SNR,
              mcs,
              nb_rb,
              PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->TBS,
              rate,
              errs[0],
              round_trials[0],
              errs[1],
              round_trials[1],
              errs[2],
              round_trials[2],
              errs[3],
              round_trials[3]);


      if (dump_perf==1) {
        printf("UE TX function statistics (per 1ms subframe)\n\n");
        std_phy_proc_tx = sqrt((double)PHY_vars_UE->phy_proc_tx.diff_square/pow(cpu_freq_GHz,2)/pow(1000,
                               2)/PHY_vars_UE->phy_proc_tx.trials - pow((double)PHY_vars_UE->phy_proc_tx.diff/PHY_vars_UE->phy_proc_tx.trials/cpu_freq_GHz/1000,2));
        printf("Total PHY proc tx                 :%f us (%d trials)\n",(double)PHY_vars_UE->phy_proc_tx.diff/PHY_vars_UE->phy_proc_tx.trials/cpu_freq_GHz/1000.0,PHY_vars_UE->phy_proc_tx.trials);
        printf("|__ Statistics                         std: %f us max: %fus min: %fus median %fus q1 %fus q3 %fus n_dropped: %d packet \n",std_phy_proc_tx, t_tx_max, t_tx_min, tx_median, tx_q1, tx_q3,
               n_tx_dropped);
        std_phy_proc_tx_ifft = sqrt((double)PHY_vars_UE->ofdm_mod_stats.diff_square/pow(cpu_freq_GHz,2)/pow(1000,
                                    2)/PHY_vars_UE->ofdm_mod_stats.trials - pow((double)PHY_vars_UE->ofdm_mod_stats.diff/PHY_vars_UE->ofdm_mod_stats.trials/cpu_freq_GHz/1000,2));
        printf("OFDM_mod time                     :%f us (%d trials)\n",(double)PHY_vars_UE->ofdm_mod_stats.diff/PHY_vars_UE->ofdm_mod_stats.trials/cpu_freq_GHz/1000.0,PHY_vars_UE->ofdm_mod_stats.trials);
        printf("|__ Statistics                         std: %f us median %fus q1 %fus q3 %fus \n",std_phy_proc_tx_ifft, tx_ifft_median, tx_ifft_q1, tx_ifft_q3);
        std_phy_proc_tx_mod = sqrt((double)PHY_vars_UE->ulsch_modulation_stats.diff_square/pow(cpu_freq_GHz,2)/pow(1000,
                                   2)/PHY_vars_UE->ulsch_modulation_stats.trials - pow((double)PHY_vars_UE->ulsch_modulation_stats.diff/PHY_vars_UE->ulsch_modulation_stats.trials/cpu_freq_GHz/1000,2));
        printf("ULSCH modulation time             :%f us (%d trials)\n",(double)PHY_vars_UE->ulsch_modulation_stats.diff/PHY_vars_UE->ulsch_modulation_stats.trials/cpu_freq_GHz/1000.0,
               PHY_vars_UE->ulsch_modulation_stats.trials);
        printf("|__ Statistics                         std: %f us median %fus q1 %fus q3 %fus \n",std_phy_proc_tx_mod, tx_mod_median, tx_mod_q1, tx_mod_q3);
        std_phy_proc_tx_enc = sqrt((double)PHY_vars_UE->ulsch_encoding_stats.diff_square/pow(cpu_freq_GHz,2)/pow(1000,
                                   2)/PHY_vars_UE->ulsch_encoding_stats.trials - pow((double)PHY_vars_UE->ulsch_encoding_stats.diff/PHY_vars_UE->ulsch_encoding_stats.trials/cpu_freq_GHz/1000,2));
        printf("ULSCH encoding time               :%f us (%d trials)\n",(double)PHY_vars_UE->ulsch_encoding_stats.diff/PHY_vars_UE->ulsch_encoding_stats.trials/cpu_freq_GHz/1000.0,
               PHY_vars_UE->ulsch_encoding_stats.trials);
        printf("|__ Statistics                         std: %f us median %fus q1 %fus q3 %fus \n",std_phy_proc_tx_enc, tx_enc_median, tx_enc_q1, tx_enc_q3);
        printf("|__ ULSCH segmentation time           :%f us (%d trials)\n",(double)PHY_vars_UE->ulsch_segmentation_stats.diff/PHY_vars_UE->ulsch_segmentation_stats.trials/cpu_freq_GHz/1000.0,
               PHY_vars_UE->ulsch_segmentation_stats.trials);
        printf("|__ ULSCH turbo encoding time         :%f us (%d trials)\n",
               ((double)PHY_vars_UE->ulsch_turbo_encoding_stats.trials/PHY_vars_UE->ulsch_encoding_stats.trials)*(double)
               PHY_vars_UE->ulsch_turbo_encoding_stats.diff/PHY_vars_UE->ulsch_turbo_encoding_stats.trials/cpu_freq_GHz/1000.0,PHY_vars_UE->ulsch_turbo_encoding_stats.trials);
        printf("|__ ULSCH rate-matching time          :%f us (%d trials)\n",
               ((double)PHY_vars_UE->ulsch_rate_matching_stats.trials/PHY_vars_UE->ulsch_encoding_stats.trials)*(double)
               PHY_vars_UE->ulsch_rate_matching_stats.diff/PHY_vars_UE->ulsch_rate_matching_stats.trials/cpu_freq_GHz/1000.0,PHY_vars_UE->ulsch_rate_matching_stats.trials);
        printf("|__ ULSCH sub-block interleaving time :%f us (%d trials)\n",
               ((double)PHY_vars_UE->ulsch_interleaving_stats.trials/PHY_vars_UE->ulsch_encoding_stats.trials)*(double)
               PHY_vars_UE->ulsch_interleaving_stats.diff/PHY_vars_UE->ulsch_interleaving_stats.trials/cpu_freq_GHz/1000.0,PHY_vars_UE->ulsch_interleaving_stats.trials);
        printf("|__ ULSCH multiplexing time           :%f us (%d trials)\n",
               ((double)PHY_vars_UE->ulsch_multiplexing_stats.trials/PHY_vars_UE->ulsch_encoding_stats.trials)*(double)
               PHY_vars_UE->ulsch_multiplexing_stats.diff/PHY_vars_UE->ulsch_multiplexing_stats.trials/cpu_freq_GHz/1000.0,PHY_vars_UE->ulsch_multiplexing_stats.trials);

        printf("\n\neNB RX function statistics (per 1ms subframe)\n\n");
        std_phy_proc_rx = sqrt((double)PHY_vars_eNB->phy_proc_rx.diff_square/pow(cpu_freq_GHz,2)/pow(1000,
                               2)/PHY_vars_eNB->phy_proc_rx.trials - pow((double)PHY_vars_eNB->phy_proc_rx.diff/PHY_vars_eNB->phy_proc_rx.trials/cpu_freq_GHz/1000,2));
        printf("Total PHY proc rx                  :%f us (%d trials)\n",(double)PHY_vars_eNB->phy_proc_rx.diff/PHY_vars_eNB->phy_proc_rx.trials/cpu_freq_GHz/1000.0,PHY_vars_eNB->phy_proc_rx.trials);
        printf("|__ Statistcs                           std: %fus max: %fus min: %fus median %fus q1 %fus q3 %fus n_dropped: %d packet \n", std_phy_proc_rx, t_rx_max, t_rx_min, rx_median, rx_q1, rx_q3,
               n_rx_dropped);
        std_phy_proc_rx_fft = sqrt((double)PHY_vars_eNB->ofdm_demod_stats.diff_square/pow(cpu_freq_GHz,2)/pow(1000,
                                   2)/PHY_vars_eNB->ofdm_demod_stats.trials - pow((double)PHY_vars_eNB->ofdm_demod_stats.diff/PHY_vars_eNB->ofdm_demod_stats.trials/cpu_freq_GHz/1000,2));
        printf("OFDM_demod time                   :%f us (%d trials)\n",(double)PHY_vars_eNB->ofdm_demod_stats.diff/PHY_vars_eNB->ofdm_demod_stats.trials/cpu_freq_GHz/1000.0,
               PHY_vars_eNB->ofdm_demod_stats.trials);
        printf("|__ Statistcs                           std: %fus median %fus q1 %fus q3 %fus \n", std_phy_proc_rx_fft, rx_fft_median, rx_fft_q1, rx_fft_q3);
        std_phy_proc_rx_demod = sqrt((double)PHY_vars_eNB->ulsch_demodulation_stats.diff_square/pow(cpu_freq_GHz,2)/pow(1000,
                                     2)/PHY_vars_eNB->ulsch_demodulation_stats.trials - pow((double)PHY_vars_eNB->ulsch_demodulation_stats.diff/PHY_vars_eNB->ulsch_demodulation_stats.trials/cpu_freq_GHz/1000,2));
        printf("ULSCH demodulation time           :%f us (%d trials)\n",(double)PHY_vars_eNB->ulsch_demodulation_stats.diff/PHY_vars_eNB->ulsch_demodulation_stats.trials/cpu_freq_GHz/1000.0,
               PHY_vars_eNB->ulsch_demodulation_stats.trials);
        printf("|__ Statistcs                           std: %fus median %fus q1 %fus q3 %fus \n", std_phy_proc_rx_demod, rx_demod_median, rx_demod_q1, rx_demod_q3);
        std_phy_proc_rx_dec = sqrt((double)PHY_vars_eNB->ulsch_decoding_stats.diff_square/pow(cpu_freq_GHz,2)/pow(1000,
                                   2)/PHY_vars_eNB->ulsch_decoding_stats.trials - pow((double)PHY_vars_eNB->ulsch_decoding_stats.diff/PHY_vars_eNB->ulsch_decoding_stats.trials/cpu_freq_GHz/1000,2));
        printf("ULSCH Decoding time (%.2f Mbit/s, avg iter %f)      :%f us (%d trials, max %f)\n",
               PHY_vars_UE->ulsch_ue[0]->harq_processes[harq_pid]->TBS/1000.0,(double)avg_iter/iter_trials,
               (double)PHY_vars_eNB->ulsch_decoding_stats.diff/PHY_vars_eNB->ulsch_decoding_stats.trials/cpu_freq_GHz/1000.0,PHY_vars_eNB->ulsch_decoding_stats.trials,
               (double)PHY_vars_eNB->ulsch_decoding_stats.max/cpu_freq_GHz/1000.0);
        printf("|__ Statistcs                           std: %fus median %fus q1 %fus q3 %fus \n", std_phy_proc_rx_dec, rx_dec_median, rx_dec_q1, rx_dec_q3);
        printf("|__ sub-block interleaving                          %f us (%d trials)\n",
               (double)PHY_vars_eNB->ulsch_deinterleaving_stats.diff/PHY_vars_eNB->ulsch_deinterleaving_stats.trials/cpu_freq_GHz/1000.0,PHY_vars_eNB->ulsch_deinterleaving_stats.trials);
        printf("|__ demultiplexing                                  %f us (%d trials)\n",
               (double)PHY_vars_eNB->ulsch_demultiplexing_stats.diff/PHY_vars_eNB->ulsch_demultiplexing_stats.trials/cpu_freq_GHz/1000.0,PHY_vars_eNB->ulsch_demultiplexing_stats.trials);
        printf("|__ rate-matching                                   %f us (%d trials)\n",
               (double)PHY_vars_eNB->ulsch_rate_unmatching_stats.diff/PHY_vars_eNB->ulsch_rate_unmatching_stats.trials/cpu_freq_GHz/1000.0,PHY_vars_eNB->ulsch_rate_unmatching_stats.trials);
        printf("|__ turbo_decoder(%d bits)                              %f us (%d cycles, %d trials)\n",
               PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->Cminus ? PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->Kminus : PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->Kplus,
               (double)PHY_vars_eNB->ulsch_turbo_decoding_stats.diff/PHY_vars_eNB->ulsch_turbo_decoding_stats.trials/cpu_freq_GHz/1000.0,
               (int)((double)PHY_vars_eNB->ulsch_turbo_decoding_stats.diff/PHY_vars_eNB->ulsch_turbo_decoding_stats.trials),PHY_vars_eNB->ulsch_turbo_decoding_stats.trials);
        printf("    |__ init                                            %f us (cycles/iter %f, %d trials)\n",
               (double)PHY_vars_eNB->ulsch_tc_init_stats.diff/PHY_vars_eNB->ulsch_tc_init_stats.trials/cpu_freq_GHz/1000.0,
               (double)PHY_vars_eNB->ulsch_tc_init_stats.diff/PHY_vars_eNB->ulsch_tc_init_stats.trials/((double)avg_iter/iter_trials),
               PHY_vars_eNB->ulsch_tc_init_stats.trials);
        printf("    |__ alpha                                           %f us (cycles/iter %f, %d trials)\n",
               (double)PHY_vars_eNB->ulsch_tc_alpha_stats.diff/PHY_vars_eNB->ulsch_tc_alpha_stats.trials/cpu_freq_GHz/1000.0,
               (double)PHY_vars_eNB->ulsch_tc_alpha_stats.diff/PHY_vars_eNB->ulsch_tc_alpha_stats.trials*2,
               PHY_vars_eNB->ulsch_tc_alpha_stats.trials);
        printf("    |__ beta                                            %f us (cycles/iter %f,%d trials)\n",
               (double)PHY_vars_eNB->ulsch_tc_beta_stats.diff/PHY_vars_eNB->ulsch_tc_beta_stats.trials/cpu_freq_GHz/1000.0,
               (double)PHY_vars_eNB->ulsch_tc_beta_stats.diff/PHY_vars_eNB->ulsch_tc_beta_stats.trials*2,
               PHY_vars_eNB->ulsch_tc_beta_stats.trials);
        printf("    |__ gamma                                           %f us (cycles/iter %f,%d trials)\n",
               (double)PHY_vars_eNB->ulsch_tc_gamma_stats.diff/PHY_vars_eNB->ulsch_tc_gamma_stats.trials/cpu_freq_GHz/1000.0,
               (double)PHY_vars_eNB->ulsch_tc_gamma_stats.diff/PHY_vars_eNB->ulsch_tc_gamma_stats.trials*2,
               PHY_vars_eNB->ulsch_tc_gamma_stats.trials);
        printf("    |__ ext                                             %f us (cycles/iter %f,%d trials)\n",
               (double)PHY_vars_eNB->ulsch_tc_ext_stats.diff/PHY_vars_eNB->ulsch_tc_ext_stats.trials/cpu_freq_GHz/1000.0,
               (double)PHY_vars_eNB->ulsch_tc_ext_stats.diff/PHY_vars_eNB->ulsch_tc_ext_stats.trials*2,
               PHY_vars_eNB->ulsch_tc_ext_stats.trials);
        printf("    |__ intl1                                           %f us (cycles/iter %f,%d trials)\n",
               (double)PHY_vars_eNB->ulsch_tc_intl1_stats.diff/PHY_vars_eNB->ulsch_tc_intl1_stats.trials/cpu_freq_GHz/1000.0,
               (double)PHY_vars_eNB->ulsch_tc_intl1_stats.diff/PHY_vars_eNB->ulsch_tc_intl1_stats.trials,
               PHY_vars_eNB->ulsch_tc_intl1_stats.trials);
        printf("    |__ intl2+HD+CRC                                    %f us (cycles/iter %f,%d trials)\n",
               (double)PHY_vars_eNB->ulsch_tc_intl2_stats.diff/PHY_vars_eNB->ulsch_tc_intl2_stats.trials/cpu_freq_GHz/1000.0,
               (double)PHY_vars_eNB->ulsch_tc_intl2_stats.diff/PHY_vars_eNB->ulsch_tc_intl2_stats.trials,
               PHY_vars_eNB->ulsch_tc_intl2_stats.trials);
      }

      if(abstx) { //ABSTRACTION
        blerr= (double)errs[1]/(round_trials[1]);
        //printf("hata yok XX,");


        blerr = (double)errs[0]/(round_trials[0]);

        if(saving_bler==0)
          fprintf(csv_fdUL,"%e;\n",blerr);

        //    printf("hata yok XX,");


        if(blerr<1)
          saving_bler = 0;
        else saving_bler =1;


      } //ABStraction

      if ( (test_perf != 0) && (100 * effective_rate > test_perf )) {
        //fprintf(time_meas_fd,"SNR; MCS; TBS; rate; err0; trials0; err1; trials1; err2; trials2; err3; trials3\n");
        fprintf(time_meas_fd,"%f;%d;%d;%f;%d;%d;%d;%d;%d;%d;%d;%d;",
                SNR,
                mcs,
                PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->TBS,
                rate,
                errs[0],
                round_trials[0],
                errs[1],
                round_trials[1],
                errs[2],
                round_trials[2],
                errs[3],
                round_trials[3]);

        //fprintf(time_meas_fd,"SNR; MCS; TBS; rate; err0; trials0; err1; trials1; err2; trials2; err3; trials3;ND;\n");
        fprintf(time_meas_fd,"%f;%d;%d;%f;%2.1f;%f;%d;%d;%d;%d;%d;%d;%d;%d;%e;%e;%e;%e;%f;%f;",
                SNR,
                mcs,
                PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->TBS,
                rate*effective_rate,
                100*effective_rate,
                rate,
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
                (1.0*(round_trials[0]-errs[0])+2.0*(round_trials[1]-errs[1])+3.0*(round_trials[2]-errs[2])+4.0*(round_trials[3]-errs[3]))/((double)round_trials[0])/
                (double)PHY_vars_eNB->ulsch_eNB[0]->harq_processes[harq_pid]->TBS,
                (1.0*(round_trials[0]-errs[0])+2.0*(round_trials[1]-errs[1])+3.0*(round_trials[2]-errs[2])+4.0*(round_trials[3]-errs[3]))/((double)round_trials[0]));

        //fprintf(time_meas_fd,"UE_PROC_TX(%d); OFDM_MOD(%d); UL_MOD(%d); UL_ENC(%d); eNB_PROC_RX(%d); OFDM_DEMOD(%d); UL_DEMOD(%d); UL_DECOD(%d);\n",
        fprintf(time_meas_fd,"%d; %d; %d; %d; %d; %d; %d; %d;",
                PHY_vars_UE->phy_proc_tx.trials,
                PHY_vars_UE->ofdm_mod_stats.trials,
                PHY_vars_UE->ulsch_modulation_stats.trials,
                PHY_vars_UE->ulsch_encoding_stats.trials,
                PHY_vars_eNB->phy_proc_rx.trials,
                PHY_vars_eNB->ofdm_demod_stats.trials,
                PHY_vars_eNB->ulsch_demodulation_stats.trials,
                PHY_vars_eNB->ulsch_decoding_stats.trials
               );
        fprintf(time_meas_fd,"%f;%f;%f;%f;%f;%f;%f;%f;",
                get_time_meas_us(&PHY_vars_UE->phy_proc_tx),
                get_time_meas_us(&PHY_vars_UE->ofdm_mod_stats),
                get_time_meas_us(&PHY_vars_UE->ulsch_modulation_stats),
                get_time_meas_us(&PHY_vars_UE->ulsch_encoding_stats),
                get_time_meas_us(&PHY_vars_eNB->phy_proc_rx),
                get_time_meas_us(&PHY_vars_eNB->ofdm_demod_stats),
                get_time_meas_us(&PHY_vars_eNB->ulsch_demodulation_stats),
                get_time_meas_us(&PHY_vars_eNB->ulsch_decoding_stats)
               );

        //fprintf(time_meas_fd,"UE_PROC_TX_STD;UE_PROC_TX_MAX;UE_PROC_TX_MIN;UE_PROC_TX_MED;UE_PROC_TX_Q1;UE_PROC_TX_Q3;UE_PROC_TX_DROPPED;\n");
        fprintf(time_meas_fd,"%f;%f;%f;%f;%f;%f;%d;", std_phy_proc_tx, t_tx_max, t_tx_min, tx_median, tx_q1, tx_q3, n_tx_dropped);

        //fprintf(time_meas_fd,"IFFT;\n");
        fprintf(time_meas_fd,"%f;%f;%f;%f;", std_phy_proc_tx_ifft, tx_ifft_median, tx_ifft_q1, tx_ifft_q3);

        //fprintf(time_meas_fd,"MOD;\n");
        fprintf(time_meas_fd,"%f;%f;%f;%f;", std_phy_proc_tx_mod, tx_mod_median, tx_mod_q1, tx_mod_q3);

        //fprintf(time_meas_fd,"ENC;\n");
        fprintf(time_meas_fd,"%f;%f;%f;%f;", std_phy_proc_tx_enc, tx_enc_median, tx_enc_q1, tx_enc_q3);

        //fprintf(time_meas_fd,"eNB_PROC_RX_STD;eNB_PROC_RX_MAX;eNB_PROC_RX_MIN;eNB_PROC_RX_MED;eNB_PROC_RX_Q1;eNB_PROC_RX_Q3;eNB_PROC_RX_DROPPED;\n");
        fprintf(time_meas_fd,"%f;%f;%f;%f;%f;%f;%d;", std_phy_proc_rx, t_rx_max, t_rx_min, rx_median, rx_q1, rx_q3, n_rx_dropped);

        //fprintf(time_meas_fd,"FFT;\n");
        fprintf(time_meas_fd,"%f;%f;%f;%f;", std_phy_proc_rx_fft, rx_fft_median, rx_fft_q1, rx_fft_q3);

        //fprintf(time_meas_fd,"DEMOD;\n");
        fprintf(time_meas_fd,"%f;%f;%f;%f;", std_phy_proc_rx_demod,rx_demod_median, rx_demod_q1, rx_demod_q3);

        //fprintf(time_meas_fd,"DEC;\n");
        fprintf(time_meas_fd,"%f;%f;%f;%f\n", std_phy_proc_rx_dec, rx_dec_median, rx_dec_q1, rx_dec_q3);


        printf("[passed] effective rate : %f  (%2.1f%%,%f)): log and break \n",rate*effective_rate, 100*effective_rate, rate );
        break;
      } else if (test_perf !=0 ) {
        printf("[continue] effective rate : %f  (%2.1f%%,%f)): increase snr \n",rate*effective_rate, 100*effective_rate, rate);
      }


      if (((double)errs[0]/(round_trials[0]))<1e-2)
        break;
    } // SNR

    //


    //write_output("chestim_f.m","chestf",PHY_vars_eNB->lte_eNB_pusch_vars[0]->drs_ch_estimates[0][0],300*12,2,1);
    // write_output("chestim_t.m","chestt",PHY_vars_eNB->lte_eNB_pusch_vars[0]->drs_ch_estimates_time[0][0], (frame_parms->ofdm_symbol_size)*2,2,1);

  }//ch realization

  /*
  if(abstx) {
    fperen = fopen(fperen_name,"a+");
    fprintf(fperen,"];\n");
    fclose(fperen);

    fmageren = fopen(fmageren_name,"a+");
    fprintf(fmageren,"];\n");
    fclose(fmageren);

    flogeren = fopen(flogeren_name,"a+");
    fprintf(flogeren,"];\n");
    fclose(flogeren);
  }
  */

  // ftxlev = fopen(ftxlev_name,"a+");
  //fprintf(ftxlev,"];\n");
  //fclose(ftxlev);


  //  write_output("chestim_f_dene.m","chestf",ulchestim_f_all,300*12,2,1);*/

  if(abstx) { // ABSTRACTION
    fprintf(csv_fdUL,"];");
    fclose(csv_fdUL);
  }

  fclose(bler_fd);

  if (test_perf !=0)
    fclose (time_meas_fd);

  printf("Freeing channel I/O\n");

  for (i=0; i<2; i++) {
    free(s_re[i]);
    free(s_im[i]);
    free(r_re[i]);
    free(r_im[i]);
  }

  free(s_re);
  free(s_im);
  free(r_re);
  free(r_im);

  //  lte_sync_time_free();

  return(0);

}



