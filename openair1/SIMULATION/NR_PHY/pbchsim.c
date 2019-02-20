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

#include "common/config/config_userapi.h"
#include "common/utils/LOG/log.h"
#include "common/ran_context.h" 

#include "SIMULATION/TOOLS/sim.h"
#include "SIMULATION/RF/rf.h"
#include "PHY/types.h"
#include "PHY/defs_nr_common.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/defs_gNB.h"
#include "PHY/NR_REFSIG/refsig_defs_ue.h"
#include "PHY/NR_REFSIG/nr_mod_table.h"
#include "PHY/MODULATION/modulation_eNB.h"
#include "PHY/MODULATION/modulation_UE.h"
#include "PHY/INIT/phy_init.h"
#include "PHY/NR_TRANSPORT/nr_transport.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"

#include "SCHED_NR/sched_nr.h"



PHY_VARS_gNB *gNB;
PHY_VARS_NR_UE *UE;
RAN_CONTEXT_t RC;


double cpuf;

// dummy functions
int nfapi_mode=0;
int oai_nfapi_hi_dci0_req(nfapi_hi_dci0_request_t *hi_dci0_req) { return(0);}
int oai_nfapi_tx_req(nfapi_tx_request_t *tx_req) { return(0); }

int oai_nfapi_dl_config_req(nfapi_dl_config_request_t *dl_config_req) { return(0); }

int oai_nfapi_ul_config_req(nfapi_ul_config_request_t *ul_config_req) { return(0); }

int oai_nfapi_nr_dl_config_req(nfapi_nr_dl_config_request_t *dl_config_req) {return(0);}

uint32_t from_nrarfcn(int nr_bandP,uint32_t dl_nrarfcn) {return(0);}
int32_t get_uldl_offset(int nr_bandP) {return(0);}

NR_IF_Module_t *NR_IF_Module_init(int Mod_id){return(NULL);}

void exit_function(const char* file, const char* function, const int line,const char *s) { 
   const char * msg= s==NULL ? "no comment": s;
   printf("Exiting at: %s:%d %s(), %s\n", file, line, function, msg); 
   exit(-1); 
}

// needed for some functions
PHY_VARS_NR_UE * PHY_vars_UE_g[1][1]={{NULL}};

int main(int argc, char **argv)
{

  char c;

  int i,aa;//,l;
  double sigma2, sigma2_dB=10,SNR,snr0=-2.0,snr1=2.0;
  double cfo=0;
  uint8_t snr1set=0;
  int **txdata;
  double **s_re,**s_im,**r_re,**r_im;
  double iqim = 0.0;
  double ip =0.0;
  unsigned char pbch_pdu[6];
  //  int sync_pos, sync_pos_slot;
  //  FILE *rx_frame_file;
  FILE *output_fd = NULL;
  uint8_t write_output_file=0;
  //int result;
  //int freq_offset;
  //  int subframe_offset;
  //  char fname[40], vname[40];
  int trial,n_trials=1,n_errors=0,n_errors_payload=0;
  uint8_t transmission_mode = 1,n_tx=1,n_rx=1;
  uint16_t Nid_cell=0;

  channel_desc_t *gNB2UE;

  uint8_t extended_prefix_flag=0;
  int8_t interf1=-21,interf2=-21;

  FILE *input_fd=NULL,*pbch_file_fd=NULL;

  //uint32_t nsymb,tx_lev,tx_lev1 = 0,tx_lev2 = 0;
  //char input_val_str[50],input_val_str2[50];
  //uint8_t frame_mod4,num_pdcch_symbols = 0;
  //double pbch_sinr;
  //int pbch_tx_ant;

  SCM_t channel_model=AWGN;//Rayleigh1_anticorr;


  int N_RB_DL=273,mu=1;

  unsigned char frame_type = 0;
  unsigned char pbch_phase = 0;

  int frame=0,subframe=0;
  int frame_length_complex_samples;
  int frame_length_complex_samples_no_prefix;
  NR_DL_FRAME_PARMS *frame_parms;
  nfapi_nr_config_request_t *gNB_config;

  int ret, payload_ret=0;
  int run_initial_sync=0;

  int loglvl=OAILOG_WARNING;

  float target_error_rate = 0.01;

  cpuf = get_cpu_freq_GHz();

  if ( load_configmodule(argc,argv) == 0) {
    exit_fun("[SOFTMODEM] Error, configuration module init failed\n");
  }

  randominit(0);

  while ((c = getopt (argc, argv, "f:hA:pf:g:i:j:n:o:s:S:t:x:y:z:N:F:GR:dP:IL:")) != -1) {
    switch (c) {
    case 'f':
      write_output_file=1;
      output_fd = fopen(optarg,"w");

      if (output_fd==NULL) {
        printf("Error opening %s\n",optarg);
        exit(-1);
      }

      break;

    case 'd':
      frame_type = 1;
      break;

    case 'g':
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

      default:
        msg("Unsupported channel model!\n");
        exit(-1);
      }

      break;

    case 'i':
      interf1=atoi(optarg);
      break;

    case 'j':
      interf2=atoi(optarg);
      break;

    case 'n':
      n_trials = atoi(optarg);
      break;

    case 'o':
      cfo = atof(optarg);
      msg("Setting CFO to %f Hz\n",cfo);
      break;

    case 's':
      snr0 = atof(optarg);
      msg("Setting SNR0 to %f\n",snr0);
      break;

    case 'S':
      snr1 = atof(optarg);
      snr1set=1;
      msg("Setting SNR1 to %f\n",snr1);
      break;

      /*
      case 't':
      Td= atof(optarg);
      break;
      */
    case 'p':
      extended_prefix_flag=1;
      break;

      /*
      case 'r':
      ricean_factor = pow(10,-.1*atof(optarg));
      if (ricean_factor>1) {
        printf("Ricean factor must be between 0 and 1\n");
        exit(-1);
      }
      break;
      */
    case 'x':
      transmission_mode=atoi(optarg);

      if ((transmission_mode!=1) &&
          (transmission_mode!=2) &&
          (transmission_mode!=6)) {
        msg("Unsupported transmission mode %d\n",transmission_mode);
        exit(-1);
      }

      break;

    case 'y':
      n_tx=atoi(optarg);

      if ((n_tx==0) || (n_tx>2)) {
        msg("Unsupported number of tx antennas %d\n",n_tx);
        exit(-1);
      }

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
      break;

    case 'F':
      input_fd = fopen(optarg,"r");

      if (input_fd==NULL) {
        printf("Problem with filename %s\n",optarg);
        exit(-1);
      }

      break;

    case 'P':
      pbch_phase = atoi(optarg);

      if (pbch_phase>3)
        printf("Illegal PBCH phase (0-3) got %d\n",pbch_phase);

      break;
      
    case 'I':
      run_initial_sync=1;
      target_error_rate=0.1;
      break;

    case 'L':
      loglvl = atoi(optarg);
      break;

    default:
    case 'h':
      printf("%s -h(elp) -p(extended_prefix) -N cell_id -f output_filename -F input_filename -g channel_model -n n_frames -t Delayspread -s snr0 -S snr1 -x transmission_mode -y TXant -z RXant -i Intefrence0 -j Interference1 -A interpolation_file -C(alibration offset dB) -N CellId\n",
             argv[0]);
      printf("-h This message\n");
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
      printf("-o Carrier frequency offset in Hz\n");
      printf("-N Nid_cell\n");
      printf("-R N_RB_DL\n");
      printf("-O oversampling factor (1,2,4,8,16)\n");
      printf("-A Interpolation_filname Run with Abstraction to generate Scatter plot using interpolation polynomial in file\n");
      //    printf("-C Generate Calibration information for Abstraction (effective SNR adjustment to remove Pe bias w.r.t. AWGN)\n");
      printf("-f Output filename (.txt format) for Pe/SNR results\n");
      printf("-F Input filename (.txt format) for RX conformance testing\n");
      exit (-1);
      break;
    }
  }

  logInit();
  set_glog(loglvl);
  T_stdout = 1;

  if (snr1set==0)
    snr1 = snr0+10;

  printf("Initializing gNodeB for mu %d, N_RB_DL %d\n",mu,N_RB_DL);

  RC.gNB = (PHY_VARS_gNB***) malloc(sizeof(PHY_VARS_gNB **));
  RC.gNB[0] = (PHY_VARS_gNB**) malloc(sizeof(PHY_VARS_gNB *));
  RC.gNB[0][0] = malloc(sizeof(PHY_VARS_gNB));
  gNB = RC.gNB[0][0];
  gNB_config = &gNB->gNB_config;
  frame_parms = &gNB->frame_parms; //to be initialized I suppose (maybe not necessary for PBCH)
  frame_parms->nb_antennas_tx = n_tx;
  frame_parms->nb_antennas_rx = n_rx;
  frame_parms->N_RB_DL = N_RB_DL;
  frame_parms->N_RB_UL = N_RB_DL;
  frame_parms->Nid_cell = Nid_cell;

  nr_phy_config_request_sim(gNB,N_RB_DL,N_RB_DL,mu,Nid_cell);
  phy_init_nr_gNB(gNB,0,0);

  double fs,bw,scs,eps;

  if (mu == 1 && N_RB_DL == 217) { 
    fs = 122.88e6;
    bw = 80e6;
    scs = 30000;
  }					       
  else if (mu == 1 && N_RB_DL == 245) {
    fs = 122.88e6;
    bw = 90e6;
    scs = 30000;
  }
  else if (mu == 1 && N_RB_DL == 273) {
    fs = 122.88e6;
    bw = 100e6;
    scs = 30000;
  }
  else if (mu == 1 && N_RB_DL == 106) { 
    fs = 61.44e6;
    bw = 40e6;
    scs = 30000;
  }
  else AssertFatal(1==0,"Unsupported numerology for mu %d, N_RB %d\n",mu, N_RB_DL);

  // cfo with respect to sub-carrier spacing
  eps = cfo/scs;

  // computation of integer and fractional FO to compare with estimation results
  int IFO;
  if(eps!=0.0){
	printf("Introducing a CFO of %lf relative to SCS of %d kHz\n",eps,(int)(scs/1000));
	if (eps>0)	
  	  IFO=(int)(eps+0.5);
	else
	  IFO=(int)(eps-0.5);
	printf("FFO = %lf; IFO = %d\n",eps-IFO,IFO);
  }

  gNB2UE = new_channel_desc_scm(n_tx,
                                n_rx,
                                channel_model,
 				fs, 
				bw, 
                                0,
                                0,
                                0);

  if (gNB2UE==NULL) {
    msg("Problem generating channel model. Exiting.\n");
    exit(-1);
  }

  frame_length_complex_samples = frame_parms->samples_per_subframe*NR_NUMBER_OF_SUBFRAMES_PER_FRAME;
  frame_length_complex_samples_no_prefix = frame_parms->samples_per_subframe_wCP;

  s_re = malloc(2*sizeof(double*));
  s_im = malloc(2*sizeof(double*));
  r_re = malloc(2*sizeof(double*));
  r_im = malloc(2*sizeof(double*));
  txdata = malloc(2*sizeof(int*));

  for (i=0; i<2; i++) {

    s_re[i] = malloc(frame_length_complex_samples*sizeof(double));
    bzero(s_re[i],frame_length_complex_samples*sizeof(double));
    s_im[i] = malloc(frame_length_complex_samples*sizeof(double));
    bzero(s_im[i],frame_length_complex_samples*sizeof(double));

    r_re[i] = malloc(frame_length_complex_samples*sizeof(double));
    bzero(r_re[i],frame_length_complex_samples*sizeof(double));
    r_im[i] = malloc(frame_length_complex_samples*sizeof(double));
    bzero(r_im[i],frame_length_complex_samples*sizeof(double));

    printf("Allocating %d samples for txdata\n",frame_length_complex_samples);
    txdata[i] = malloc(frame_length_complex_samples*sizeof(int));
    bzero(r_re[i],frame_length_complex_samples*sizeof(int));
  
  }

  if (pbch_file_fd!=NULL) {
    load_pbch_desc(pbch_file_fd);
  }


  //configure UE
  UE = malloc(sizeof(PHY_VARS_NR_UE));
  memcpy(&UE->frame_parms,frame_parms,sizeof(NR_DL_FRAME_PARMS));
  //phy_init_nr_top(UE); //called from init_nr_ue_signal
  if (run_initial_sync==1)  UE->is_synchronized = 0;
  else                      UE->is_synchronized = 1;
                      
  UE->perfect_ce = 0;

  if(eps!=0.0)
	UE->UE_fo_compensation = 1; // if a frequency offset is set then perform fo estimation and compensation

  if (init_nr_ue_signal(UE, 1, 0) != 0)
  {
    printf("Error at UE NR initialisation\n");
    exit(-1);
  }

  nr_gold_pbch(UE);
  // generate signal
  if (input_fd==NULL) {
    gNB->pbch_configured = 1;
    for (int i=0;i<4;i++) gNB->pbch_pdu[i]=i+1;
    nr_common_signal_procedures (gNB,frame,subframe);

	LOG_M("txsigF0.m","txsF0", gNB->common_vars.txdataF[0],frame_length_complex_samples_no_prefix,1,1);
	if (gNB->frame_parms.nb_antennas_tx>1)
	LOG_M("txsigF1.m","txsF1", gNB->common_vars.txdataF[1],frame_length_complex_samples_no_prefix,1,1);

    //TODO: loop over slots
    for (aa=0; aa<gNB->frame_parms.nb_antennas_tx; aa++) {
      if (gNB_config->subframe_config.dl_cyclic_prefix_type.value == 1) {
	PHY_ofdm_mod(gNB->common_vars.txdataF[aa],
		     txdata[aa],
		     frame_parms->ofdm_symbol_size,
		     12,
		     frame_parms->nb_prefix_samples,
		     CYCLIC_PREFIX);
      } else {
	nr_normal_prefix_mod(gNB->common_vars.txdataF[aa],
			     txdata[aa],
			     14,
			     frame_parms);
      }
    }
  } else {
    printf("Reading %d samples from file to antenna buffer %d\n",frame_length_complex_samples,0);
    
    if (fread(txdata[0],
	      sizeof(int32_t),
	      frame_length_complex_samples,
	      input_fd) != frame_length_complex_samples) {
      printf("error reading from file\n");
      //exit(-1);
    }
  }

  LOG_M("txsig0.m","txs0", txdata[0],frame_length_complex_samples,1,1);
  if (gNB->frame_parms.nb_antennas_tx>1)
    LOG_M("txsig1.m","txs1", txdata[1],frame_length_complex_samples,1,1);

  if (output_fd) 
    fwrite(txdata[0],sizeof(int32_t),frame_length_complex_samples,output_fd);

  int txlev = signal_energy(&txdata[0][5*frame_parms->ofdm_symbol_size + 4*frame_parms->nb_prefix_samples + frame_parms->nb_prefix_samples0],
			    frame_parms->ofdm_symbol_size + frame_parms->nb_prefix_samples);

  //  printf("txlev %d (%f)\n",txlev,10*log10(txlev));


  for (i=0; i<frame_length_complex_samples; i++) {
    for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
      r_re[aa][i] = ((double)(((short *)txdata[aa]))[(i<<1)]);
      r_im[aa][i] = ((double)(((short *)txdata[aa]))[(i<<1)+1]);
    }
  }
  
  for (SNR=snr0; SNR<snr1; SNR+=.2) {

    n_errors = 0;
    n_errors_payload = 0;

    for (trial=0; trial<n_trials; trial++) {

      // multipath channel
      //multipath_channel(gNB2UE,s_re,s_im,r_re,r_im,frame_length_complex_samples,0);
      
      //AWGN
      sigma2_dB = 20*log10((double)AMP/4)-SNR;
      sigma2 = pow(10,sigma2_dB/10);
      //printf("sigma2 %f (%f dB), tx_lev %f (%f dB)\n",sigma2,sigma2_dB,txlev,10*log10((double)txlev));

      if(eps!=0.0)
        rf_rx(r_re,  // real part of txdata
           r_im,  // imag part of txdata
           NULL,  // interference real part
           NULL, // interference imag part
           0,  // interference power
           frame_parms->nb_antennas_rx,  // number of rx antennas
           frame_length_complex_samples,  // number of samples in frame
           1.0e9/fs,   //sampling time (ns)
           cfo,	// frequency offset in Hz
           0.0, // drift (not implemented)
           0.0, // noise figure (not implemented)
           0.0, // rx gain in dB ?
           200, // 3rd order non-linearity in dB ?
           &ip, // initial phase
           30.0e3,  // phase noise cutoff in kHz
           -500.0, // phase noise amplitude in dBc
           0.0,  // IQ imbalance (dB),
	   0.0); // IQ phase imbalance (rad)

   
      for (i=0; i<frame_length_complex_samples; i++) {
	for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
	  
	  ((short*) UE->common_vars.rxdata[aa])[2*i]   = (short) ((r_re[aa][i] + sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
	  ((short*) UE->common_vars.rxdata[aa])[2*i+1] = (short) ((r_im[aa][i] + sqrt(sigma2/2)*gaussdouble(0.0,1.0)));
	}
      }

      if (n_trials==1) {
	LOG_M("rxsig0.m","rxs0", UE->common_vars.rxdata[0],frame_parms->samples_per_subframe,1,1);
	if (gNB->frame_parms.nb_antennas_tx>1)
	  LOG_M("rxsig1.m","rxs1", UE->common_vars.rxdata[1],frame_parms->samples_per_subframe,1,1);
      }

      if (UE->is_synchronized == 0) {
	ret = nr_initial_sync(UE, normal_txrx);
	printf("nr_initial_sync1 returns %d\n",ret);
	if (ret<0) n_errors++;
      }
      else {
	UE->rx_offset=0;
	//symbol 1
	nr_slot_fep(UE,
		    5,
		    0,
		    0,
		    0,
		    1,
		    NR_PBCH_EST);
	
	//symbol 2
	nr_slot_fep(UE,
		    6,
		    0,
		    0,
		    0,
		    1,
		    NR_PBCH_EST);
	
	//symbol 3
	nr_slot_fep(UE,
		    7,
		    0,
		    0,
		    0,
		    1,
		    NR_PBCH_EST);
	 
	ret = nr_rx_pbch(UE,
			 &UE->proc.proc_rxtx[0],
			 UE->pbch_vars[0],
			 frame_parms,
			 0,
			 SISO,
			 UE->high_speed_flag);

	if (ret==0) {
	  //UE->rx_ind.rx_indication_body->mib_pdu.ssb_index;  //not yet detected automatically
	  //UE->rx_ind.rx_indication_body->mib_pdu.ssb_length; //Lmax, not yet detected automatically
	  uint8_t gNB_xtra_byte=0;
	  for (int i=0; i<8; i++)
	    gNB_xtra_byte |= ((gNB->pbch.pbch_a>>(31-i))&1)<<(7-i);
	  
	  payload_ret = (UE->rx_ind.rx_indication_body->mib_pdu.additional_bits == gNB_xtra_byte);
	  for (i=0;i<3;i++){
	    payload_ret += (UE->rx_ind.rx_indication_body->mib_pdu.pdu[i] == gNB->pbch_pdu[2-i]);
	    //printf("pdu byte %d gNB: 0x%02x UE: 0x%02x\n",i,gNB->pbch_pdu[i], UE->rx_ind.rx_indication_body->mib_pdu.pdu[i]); 
	  } 
	  //printf("xtra byte gNB: 0x%02x UE: 0x%02x\n",gNB_xtra_byte, UE->rx_ind.rx_indication_body->mib_pdu.additional_bits);
	  //printf("ret %d\n", payload_ret);
	  if (payload_ret!=4) 
	    n_errors_payload++;
	}

	if (ret!=0) n_errors++;
      }
    } //noise trials

    printf("SNR %f: trials %d, n_errors_crc = %d, n_errors_payload %d\n", SNR,n_trials,n_errors,n_errors_payload);

    if (((float)n_errors/(float)n_trials <= target_error_rate) && (n_errors_payload==0)) {
      printf("PBCH test OK\n");
      break;
    }
      
    if (n_trials==1)
      break;

  } // NSR

  for (i=0; i<2; i++) {
    free(s_re[i]);
    free(s_im[i]);
    free(r_re[i]);
    free(r_im[i]);
    free(txdata[i]);
  }

  free(s_re);
  free(s_im);
  free(r_re);
  free(r_im);
  free(txdata);

  if (output_fd)
    fclose(output_fd);

  if (input_fd)
    fclose(input_fd);

  return(n_errors);

}



