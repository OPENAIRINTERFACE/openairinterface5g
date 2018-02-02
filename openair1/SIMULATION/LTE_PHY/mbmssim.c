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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "SIMULATION/TOOLS/defs.h"
#include "SIMULATION/RF/defs.h"
#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/vars.h"
#include "SCHED/defs.h"
#include "SCHED/vars.h"
#include "LAYER2/MAC/vars.h"

#ifdef XFORMS
#include "PHY/TOOLS/lte_phy_scope.h"
#endif //XFORMS


#include "OCG_vars.h"
#include "unitary_defs.h"


PHY_VARS_eNB *eNB;
PHY_VARS_UE *UE;

double cpuf;

DCI1E_5MHz_2A_M10PRB_TDD_t  DLSCH_alloc_pdu2_1E[2];
#define UL_RB_ALLOC 0x1ff;
#define CCCH_RB_ALLOC computeRIV(eNB->frame_parms.N_RB_UL,0,2)
int main(int argc, char **argv)
{

  char c;

  int i,l,l2,aa,aarx,k;
  double sigma2, sigma2_dB=0,SNR,snr0=-2.0,snr1=0.0;
  uint8_t snr1set=0;
  double snr_step=1,input_snr_step=1;
  int **txdata;
  double s_re0[2*30720],s_im0[2*30720],s_re1[2*30720],s_im1[2*30720];
  double r_re0[2*30720],r_im0[2*30720],r_re1[2*30720],r_im1[2*30720];
  double *s_re[2]={s_re0,s_re1},*s_im[2]={s_im0,s_im1},*r_re[2]={r_re0,r_re1},*r_im[2]={r_im0,r_im1};
  double iqim = 0.0;
  int subframe=1;
  char fname[40];//, vname[40];
  uint8_t transmission_mode = 1,n_tx=1,n_rx=2;
  uint16_t Nid_cell=0;

  FILE *fd;

  int eNB_id = 0;
  unsigned char mcs=0,awgn_flag=0,round;

  int n_frames=1;
  channel_desc_t *eNB2UE;
  uint32_t nsymb,tx_lev,tx_lev_dB;
  uint8_t extended_prefix_flag=1;
  LTE_DL_FRAME_PARMS *frame_parms;
  int hold_channel=0;


  uint16_t NB_RB=25;

  int tdd_config=3;

  SCM_t channel_model=MBSFN;


  unsigned char *input_buffer;
  unsigned short input_buffer_length;
  unsigned int ret;

  unsigned int trials,errs[4]= {0,0,0,0}; //,round_trials[4]={0,0,0,0};

  uint8_t N_RB_DL=25,osf=1;
  uint32_t perfect_ce = 0;

  lte_frame_type_t frame_type = FDD;

  uint32_t Nsoft = 1827072;

  /*
#ifdef XFORMS
  FD_lte_phy_scope_ue *form_ue;
  char title[255];

  fl_initialize (&argc, argv, NULL, 0, 0);
  form_ue = create_lte_phy_scope_ue();
  sprintf (title, "LTE DL SCOPE UE");
  fl_show_form (form_ue->lte_phy_scope_ue, FL_PLACE_HOTSPOT, FL_FULLBORDER, title);
#endif
  */

  cpuf = get_cpu_freq_GHz();

  logInit();
  number_of_cards = 1;

  while ((c = getopt (argc, argv, "ahA:Cp:n:s:S:t:x:y:z:N:F:R:O:dm:i:Y")) != -1) {
    switch (c) {
    case 'a':
      awgn_flag=1;
      break;

    case 'd':
      frame_type = 0;
      break;

    case 'n':
      n_frames = atoi(optarg);
      break;

    case 'm':
      mcs=atoi(optarg);
      break;

    case 's':
      snr0 = atof(optarg);
      msg("Setting SNR0 to %f\n",snr0);
      break;

    case 'i':
      input_snr_step = atof(optarg);
      break;

    case 'S':
      snr1 = atof(optarg);
      snr1set=1;
      msg("Setting SNR1 to %f\n",snr1);
      break;

    case 'p': // subframe no;
      subframe=atoi(optarg);
      break;

    case 'z':
      n_rx=atoi(optarg);

      if ((n_rx==0) || (n_rx>2)) {
        msg("Unsupported number of rx antennas %d\n",n_rx);
        exit(-1);
      }

      break;

    case 'N':
      Nid_cell = atoi(optarg);
      break;

    case 'R':
      N_RB_DL = atoi(optarg);

      if ((N_RB_DL!=6) && (N_RB_DL!=25) && (N_RB_DL!=50) && (N_RB_DL!=100))  {
        printf("Unsupported Bandwidth %d\n",N_RB_DL);
        exit(-1);
      }

      break;

    case 'O':
      osf = atoi(optarg);
      break;

    case 'Y':
      perfect_ce = 1;
      break;

    default:
    case 'h':
      printf("%s -h(elp) -p(subframe) -N cell_id -g channel_model -n n_frames -t Delayspread -s snr0 -S snr1 -i snr increment -z RXant \n",argv[0]);
      printf("-h This message\n");
      printf("-a Use AWGN Channel\n");
      printf("-p Use extended prefix mode\n");
      printf("-d Use TDD\n");
      printf("-n Number of frames to simulate\n");
      printf("-s Starting SNR, runs from SNR0 to SNR0 + 5 dB.  If n_frames is 1 then just SNR is simulated\n");
      printf("-S Ending SNR, runs from SNR0 to SNR1\n");
      printf("-t Delay spread for multipath channel\n");
      printf("-g [A,B,C,D,E,F,G] Use 3GPP SCM (A,B,C,D) or 36-101 (E-EPA,F-EVA,G-ETU) models (ignores delay spread and Ricean factor)\n");
      printf("-x Transmission mode (1,2,6 for the moment)\n");
      printf("-y Number of TX antennas used in eNB\n");
      printf("-z Number of RX antennas used in UE\n");
      printf("-i Relative strength of first intefering eNB (in dB) - cell_id mod 3 = 1\n");
      printf("-j Relative strength of second intefering eNB (in dB) - cell_id mod 3 = 2\n");
      printf("-N Nid_cell\n");
      printf("-R N_RB_DL\n");
      printf("-O oversampling factor (1,2,4,8,16)\n");
      printf("-A Interpolation_filname Run with Abstraction to generate Scatter plot using interpolation polynomial in file\n");
      printf("-C Generate Calibration information for Abstraction (effective SNR adjustment to remove Pe bias w.r.t. AWGN)\n");
      printf("-f Output filename (.txt format) for Pe/SNR results\n");
      printf("-F Input filename (.txt format) for RX conformance testing\n");
      exit (-1);
      break;
    }
  }



  if (awgn_flag == 1)
    channel_model = AWGN;

  // check that subframe is legal for eMBMS

  if ((subframe == 0) || (subframe == 5) ||    // TDD and FDD SFn 0,5;
      ((frame_type == FDD) && ((subframe == 4) || (subframe == 9))) || // FDD SFn 4,9;
      ((frame_type == TDD ) && ((subframe<3) || (subframe==6))))    {  // TDD SFn 1,2,6;

    printf("Illegal subframe %d for eMBMS transmission (frame_type %d)\n",subframe,frame_type);
    exit(-1);
  }

  if (transmission_mode==2)
    n_tx=2;

  lte_param_init(n_tx,
                 n_tx,
		 n_rx,
		 transmission_mode,
		 extended_prefix_flag,
		 frame_type,
		 Nid_cell,
		 tdd_config,
		 N_RB_DL,
		 0,
		 osf,
		 perfect_ce);

  if (snr1set==0) {
    if (n_frames==1)
      snr1 = snr0+.1;
    else
      snr1 = snr0+5.0;
  }

  printf("SNR0 %f, SNR1 %f\n",snr0,snr1);

  frame_parms = &eNB->frame_parms;

  if (awgn_flag == 0)
    sprintf(fname,"embms_%d_%d.m",mcs,N_RB_DL);
  else
    sprintf(fname,"embms_awgn_%d_%d.m",mcs,N_RB_DL);

  if (!(fd = fopen(fname,"w"))) {
    printf("Cannot open %s, check permissions\n",fname);
    exit(-1);
  }
	

  if (awgn_flag==0)
    fprintf(fd,"SNR_%d_%d=[];errs_mch_%d_%d=[];mch_trials_%d_%d=[];\n",
            mcs,N_RB_DL,
            mcs,N_RB_DL,
            mcs,N_RB_DL);
  else
    fprintf(fd,"SNR_awgn_%d_%d=[];errs_mch_awgn_%d_%d=[];mch_trials_awgn_%d_%d=[];\n",
            mcs,N_RB_DL,
            mcs,N_RB_DL,
            mcs,N_RB_DL);

  fflush(fd);

  txdata = eNB->common_vars.txdata[0];

  nsymb = 12;

  printf("FFT Size %d, Extended Prefix %d, Samples per subframe %d, Symbols per subframe %d, AWGN %d\n",NUMBER_OF_OFDM_CARRIERS,
         frame_parms->Ncp,frame_parms->samples_per_tti,nsymb,awgn_flag);

  eNB2UE = new_channel_desc_scm(eNB->frame_parms.nb_antennas_tx,
                                UE->frame_parms.nb_antennas_rx,
                                channel_model,
				N_RB2sampling_rate(eNB->frame_parms.N_RB_DL),
				N_RB2channel_bandwidth(eNB->frame_parms.N_RB_DL),
                                0,
                                0,
                                0);

  // Create transport channel structures for 2 transport blocks (MIMO)
  eNB->dlsch_MCH = new_eNB_dlsch(1,8,Nsoft,N_RB_DL,0,&eNB->frame_parms);

  if (!eNB->dlsch_MCH) {
    printf("Can't get eNB dlsch structures\n");
    exit(-1);
  }

  UE->dlsch_MCH[0]  = new_ue_dlsch(1,8,Nsoft,MAX_TURBO_ITERATIONS_MBSFN,N_RB_DL,0);

  eNB->frame_parms.num_MBSFN_config = 1;
  eNB->frame_parms.MBSFN_config[0].radioframeAllocationPeriod = 0;
  eNB->frame_parms.MBSFN_config[0].radioframeAllocationOffset = 0;
  eNB->frame_parms.MBSFN_config[0].fourFrames_flag = 0;
  eNB->frame_parms.MBSFN_config[0].mbsfn_SubframeConfig=0xff; // activate all possible subframes
  UE->frame_parms.num_MBSFN_config = 1;
  UE->frame_parms.MBSFN_config[0].radioframeAllocationPeriod = 0;
  UE->frame_parms.MBSFN_config[0].radioframeAllocationOffset = 0;
  UE->frame_parms.MBSFN_config[0].fourFrames_flag = 0;
  UE->frame_parms.MBSFN_config[0].mbsfn_SubframeConfig=0xff; // activate all possible subframes

  fill_eNB_dlsch_MCH(eNB,mcs,1,0);
  fill_UE_dlsch_MCH(UE,mcs,1,0,0);

  if (is_pmch_subframe(0,subframe,&eNB->frame_parms)==0) {
    printf("eNB is not configured for MBSFN in subframe %d\n",subframe);
    exit(-1);
  } else if (is_pmch_subframe(0,subframe,&UE->frame_parms)==0) {
    printf("UE is not configured for MBSFN in subframe %d\n",subframe);
    exit(-1);
  }


  input_buffer_length = eNB->dlsch_MCH->harq_processes[0]->TBS/8;
  input_buffer = (unsigned char *)malloc(input_buffer_length+4);
  memset(input_buffer,0,input_buffer_length+4);

  for (i=0; i<input_buffer_length+4; i++) {
    input_buffer[i]= (unsigned char)(taus()&0xff);
  }


  snr_step = input_snr_step;

  for (SNR=snr0; SNR<snr1; SNR+=snr_step) {
    UE->proc.proc_rxtx[0].frame_tx=0;
    eNB->proc.proc_rxtx[0].frame_tx=0;
    eNB->proc.proc_rxtx[0].subframe_tx=subframe;

    errs[0]=0;
    errs[1]=0;
    errs[2]=0;
    errs[3]=0;
    /*
    round_trials[0] = 0;
    round_trials[1] = 0;
    round_trials[2] = 0;
    round_trials[3] = 0;*/
    printf("********************** SNR %f (step %f)\n",SNR,snr_step);

    for (trials = 0; trials<n_frames; trials++) {
      //        printf("Trial %d\n",trials);
      fflush(stdout);
      round=0;

      //if (trials%100==0)
      //eNB2UE[0]->first_run = 1;
      eNB2UE->first_run = 1;
      memset(&eNB->common_vars.txdataF[0][0][0],0,FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX*sizeof(int32_t));
      generate_mch(eNB,&eNB->proc.proc_rxtx[0],input_buffer);


      PHY_ofdm_mod(eNB->common_vars.txdataF[0][0],        // input,
                   txdata[0],         // output
                   frame_parms->ofdm_symbol_size,
                   LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*nsymb,                 // number of symbols
                   frame_parms->nb_prefix_samples,               // number of prefix samples
                   CYCLIC_PREFIX);

      if (n_frames==1) {
        write_output("txsigF0.m","txsF0", &eNB->common_vars.txdataF[0][0][subframe*nsymb*eNB->frame_parms.ofdm_symbol_size],
                     nsymb*eNB->frame_parms.ofdm_symbol_size,1,1);
        //if (eNB->frame_parms.nb_antennas_tx>1)
        //write_output("txsigF1.m","txsF1", &eNB->lte_eNB_common_vars.txdataF[eNB_id][1][subframe*nsymb*eNB->frame_parms.ofdm_symbol_size],nsymb*eNB->frame_parms.ofdm_symbol_size,1,1);
      }

      tx_lev = 0;

      for (aa=0; aa<eNB->frame_parms.nb_antennas_tx; aa++) {
        tx_lev += signal_energy(&eNB->common_vars.txdata[eNB_id][aa]
                                [subframe*eNB->frame_parms.samples_per_tti],
                                eNB->frame_parms.samples_per_tti);
      }

      tx_lev_dB = (unsigned int) dB_fixed(tx_lev);

      if (n_frames==1) {
        printf("tx_lev = %d (%d dB)\n",tx_lev,tx_lev_dB);
        //    write_output("txsig0.m","txs0", &eNB->common_vars.txdata[0][0][subframe* eNB->frame_parms.samples_per_tti],

        //     eNB->frame_parms.samples_per_tti,1,1);
      }


      for (i=0; i<2*frame_parms->samples_per_tti; i++) {
        for (aa=0; aa<eNB->frame_parms.nb_antennas_tx; aa++) {
          s_re[aa][i] = ((double)(((short *)eNB->common_vars.txdata[0][aa]))[(2*subframe*UE->frame_parms.samples_per_tti) + (i<<1)]);
          s_im[aa][i] = ((double)(((short *)eNB->common_vars.txdata[0][aa]))[(2*subframe*UE->frame_parms.samples_per_tti) +(i<<1)+1]);
        }
      }

      //Multipath channel
      multipath_channel(eNB2UE,s_re,s_im,r_re,r_im,
                        2*frame_parms->samples_per_tti,hold_channel);

      //AWGN
      sigma2_dB = 10*log10((double)tx_lev) +10*log10((double)eNB->frame_parms.ofdm_symbol_size/(NB_RB*12)) - SNR;
      sigma2 = pow(10,sigma2_dB/10);

      if (n_frames==1)
        printf("Sigma2 %f (sigma2_dB %f)\n",sigma2,sigma2_dB);

      for (i=0; i<2*frame_parms->samples_per_tti; i++) {
        for (aa=0; aa<eNB->frame_parms.nb_antennas_rx; aa++) {
          //printf("s_re[0][%d]=> %f , r_re[0][%d]=> %f\n",i,s_re[aa][i],i,r_re[aa][i]);
          ((short*) UE->common_vars.rxdata[aa])[(2*subframe*UE->frame_parms.samples_per_tti)+2*i] =
            (short) (r_re[aa][i] + sqrt(sigma2/2)*gaussdouble(0.0,1.0));
          ((short*) UE->common_vars.rxdata[aa])[(2*subframe*UE->frame_parms.samples_per_tti)+2*i+1] =
            (short) (r_im[aa][i] + (iqim*r_re[aa][i]) + sqrt(sigma2/2)*gaussdouble(0.0,1.0));
        }
      }

      for (l=2; l<12; l++) {

        slot_fep_mbsfn(UE,
                       l,
                       subframe%10,
                       0,
                       0);
	if (UE->perfect_ce==1) {
	  // fill in perfect channel estimates
	  freq_channel(eNB2UE,UE->frame_parms.N_RB_DL,12*UE->frame_parms.N_RB_DL + 1);
	  for(k=0; k<NUMBER_OF_eNB_MAX; k++) {
	    for(aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
	      for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
		for (i=0; i<frame_parms->N_RB_DL*12; i++) {
		  ((int16_t *) UE->common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[k][(aa<<1)+aarx])[2*i+(l*frame_parms->ofdm_symbol_size+LTE_CE_FILTER_LENGTH)*2]=(int16_t)(eNB2UE->chF[aarx+(aa*frame_parms->nb_antennas_rx)][i].x*AMP);
		  ((int16_t *) UE->common_vars.common_vars_rx_data_per_thread[subframe&0x1].dl_ch_estimates[k][(aa<<1)+aarx])[2*i+1+(l*frame_parms->ofdm_symbol_size+LTE_CE_FILTER_LENGTH)*2]=(int16_t)(eNB2UE->chF[aarx+(aa*frame_parms->nb_antennas_rx)][i].y*AMP);
		}
	      }
	    }
	  }
	}

	if (l==6)
          for (l2=2;l2<7;l2++)
	    rx_pmch(UE,
		    0,
		    subframe%10,
		    l2);
	if (l==6)
          for (l2=2;l2<7;l2++)
	    rx_pmch(UE,
		    0,
		    subframe%10,
		    l2);
	if (l==11)
          for (l2=7;l2<12;l2++)
	    rx_pmch(UE,
		    0,
		    subframe%10,
		    l2);
      }

      UE->dlsch_MCH[0]->harq_processes[0]->G = get_G(&UE->frame_parms,
						     UE->dlsch_MCH[0]->harq_processes[0]->nb_rb,
						     UE->dlsch_MCH[0]->harq_processes[0]->rb_alloc_even,
						     get_Qm(UE->dlsch_MCH[0]->harq_processes[0]->mcs),
						     1,2,
						     UE->proc.proc_rxtx[0].frame_tx,subframe,0);
      UE->dlsch_MCH[0]->harq_processes[0]->Qm = get_Qm(UE->dlsch_MCH[0]->harq_processes[0]->mcs);

      dlsch_unscrambling(&UE->frame_parms,1,UE->dlsch_MCH[0],
                         UE->dlsch_MCH[0]->harq_processes[0]->G,
                         UE->pdsch_vars_MCH[0]->llr[0],0,subframe<<1);

      ret = dlsch_decoding(UE,
                           UE->pdsch_vars_MCH[0]->llr[0],
                           &UE->frame_parms,
                           UE->dlsch_MCH[0],
                           UE->dlsch_MCH[0]->harq_processes[0],
                           trials,
                           subframe,
                           0,0,0);

      if (n_frames==1)
        printf("MCH decoding returns %d\n",ret);

      if (ret == (1+UE->dlsch_MCH[0]->max_turbo_iterations))
        errs[0]++;

      UE->proc.proc_rxtx[0].frame_tx++;
      eNB->proc.proc_rxtx[0].frame_tx++;
    }

    printf("errors %d/%d (Pe %e)\n",errs[round],trials,(double)errs[round]/trials);

    if (awgn_flag==0)
      fprintf(fd,"SNR_%d_%d = [SNR_%d_%d %f]; errs_mch_%d_%d =[errs_mch_%d_%d  %d]; mch_trials_%d_%d =[mch_trials_%d_%d  %d];\n",
              mcs,N_RB_DL,mcs,N_RB_DL,SNR,
              mcs,N_RB_DL,mcs,N_RB_DL,errs[0],
              mcs,N_RB_DL,mcs,N_RB_DL,trials);
    else
      fprintf(fd,"SNR_awgn_%d = [SNR_awgn_%d %f]; errs_mch_awgn_%d =[errs_mch_awgn_%d  %d]; mch_trials_awgn_%d =[mch_trials_awgn_%d %d];\n",
              N_RB_DL,N_RB_DL,SNR,
              N_RB_DL,N_RB_DL,errs[0],
              N_RB_DL,N_RB_DL,trials);

    fflush(fd);

    if (errs[0] == 0)
      break;
  }


  if (n_frames==1) {
    printf("Dumping PMCH files ( G %d)\n",UE->dlsch_MCH[0]->harq_processes[0]->G);
    dump_mch(UE,0,
             UE->dlsch_MCH[0]->harq_processes[0]->G,
             subframe);
  }

  printf("Freeing dlsch structures\n");
  free_eNB_dlsch(eNB->dlsch_MCH);
  free_ue_dlsch(UE->dlsch_MCH[0]);

  fclose(fd);

  return(0);
}

