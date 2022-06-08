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
#include "common/ran_context.h"
#include "common/config/config_userapi.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "common/utils/load_module_shlib.h"
#include "T.h"
#include "PHY/defs_gNB.h"
#include "PHY/defs_nr_common.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/types.h"
#include "PHY/INIT/phy_init.h"
#include "PHY/MODULATION/modulation_eNB.h"
#include "PHY/MODULATION/modulation_UE.h"
#include "PHY/NR_REFSIG/refsig_defs_ue.h"
#include "PHY/NR_TRANSPORT/nr_dlsch.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "SCHED_NR/sched_nr.h"
#include "openair1/SIMULATION/TOOLS/sim.h"
#include "openair1/SIMULATION/RF/rf.h"
#include "openair1/SIMULATION/NR_PHY/nr_unitary_defs.h"
#include "openair1/SIMULATION/NR_PHY/nr_dummy_functions.c"
#include "openair2/LAYER2/NR_MAC_COMMON/nr_mac_common.h"


//#define DEBUG_NR_DLSCHSIM

THREAD_STRUCT thread_struct;
PHY_VARS_gNB *gNB;
PHY_VARS_NR_UE *UE;
RAN_CONTEXT_t RC;
UE_nr_rxtx_proc_t proc;
int32_t uplink_frequency_offset[MAX_NUM_CCs][4];
uint64_t downlink_frequency[MAX_NUM_CCs][4];

double cpuf;
//uint8_t nfapi_mode = 0;
uint16_t NB_UE_INST = 1;

uint8_t const nr_rv_round_map[4] = {0, 2, 3, 1};

// needed for some functions
PHY_VARS_NR_UE *PHY_vars_UE_g[1][1] = { { NULL } };
uint16_t n_rnti = 0x1234;
openair0_config_t openair0_cfg[MAX_CARDS];

void init_downlink_harq_status(NR_DL_UE_HARQ_t *dl_harq) {}

int main(int argc, char **argv)
{
	char c;
	int i; //,j,l,aa;
	double SNR, SNR_lin, snr0 = -2.0, snr1 = 2.0;
	double snr_step = 0.1;
	uint8_t snr1set = 0;
	int **txdata;
	double **s_re, **s_im, **r_re, **r_im;
	//  int sync_pos, sync_pos_slot;
	//  FILE *rx_frame_file;
	FILE *output_fd = NULL;
	//uint8_t write_output_file = 0;
	//  int subframe_offset;
	//  char fname[40], vname[40];
	int trial, n_trials = 1, n_errors = 0, n_false_positive = 0;
	uint8_t n_tx = 1, n_rx = 1;
	//uint8_t transmission_mode = 1;
	uint16_t Nid_cell = 0;
	channel_desc_t *gNB2UE;
	uint8_t extended_prefix_flag = 0;
	//int8_t interf1 = -21, interf2 = -21;
	FILE *input_fd = NULL, *pbch_file_fd = NULL;
	//char input_val_str[50],input_val_str2[50];
	//uint16_t NB_RB=25;
	SCM_t channel_model = AWGN;  //Rayleigh1_anticorr;
	uint16_t N_RB_DL = 106, mu = 1; 
	//unsigned char frame_type = 0;
	unsigned char pbch_phase = 0;
	int frame = 0, slot = 0;
	int frame_length_complex_samples;
	//int frame_length_complex_samples_no_prefix;
	NR_DL_FRAME_PARMS *frame_parms;
	uint8_t Kmimo = 0;
	uint32_t Nsoft = 0;
	double sigma;
	unsigned char qbits = 8;
	int ret;
	//int run_initial_sync=0;
	int loglvl = OAILOG_WARNING;
	uint8_t dlsch_threads = 0;
	float target_error_rate = 0.01;
        uint64_t SSB_positions=0x01;
	uint16_t nb_symb_sch = 12;
	uint16_t nb_rb = 50;
	uint8_t Imcs = 9;
        uint8_t mcs_table = 0;
        double DS_TDL = .03;
	cpuf = get_cpu_freq_GHz();
	char gNBthreads[128]="n";
        int Tbslbrm = 950984;

	if (load_configmodule(argc, argv, CONFIG_ENABLECMDLINEONLY) == 0) {
		exit_fun("[NR_DLSCHSIM] Error, configuration module init failed\n");
	}

	//logInit();
	randominit(0);

	while ((c = getopt(argc, argv, "df:hpVg:i:j:n:l:m:r:s:S:y:z:M:N:F:R:P:L:X:")) != -1) {
		switch (c) {
		/*case 'f':
			write_output_file = 1;
			output_fd = fopen(optarg, "w");

			if (output_fd == NULL) {
				printf("Error opening %s\n", optarg);
				exit(-1);
			}

			break;*/

		case 'd':
			dlsch_threads = atoi(optarg);
			break;

		case 'g':
			switch ((char) *optarg) {
			case 'A':
				channel_model = SCM_A;
				break;

			case 'B':
				channel_model = SCM_B;
				break;

			case 'C':
				channel_model = SCM_C;
				break;

			case 'D':
				channel_model = SCM_D;
				break;

			case 'E':
				channel_model = EPA;
				break;

			case 'F':
				channel_model = EVA;
				break;

			case 'G':
				channel_model = ETU;
				break;

			default:
				printf("Unsupported channel model! Exiting.\n");
				exit(-1);
			}

			break;

		/*case 'i':
			interf1 = atoi(optarg);
			break;

		case 'j':
			interf2 = atoi(optarg);
			break;*/

		case 'n':
			n_trials = atoi(optarg);
			break;

		case 's':
			snr0 = atof(optarg);
#ifdef DEBUG_NR_DLSCHSIM
			printf("Setting SNR0 to %f\n", snr0);
#endif
			break;

		case 'V':
		  ouput_vcd = 1;
		  break;

		case 'S':
			snr1 = atof(optarg);
			snr1set = 1;
			printf("Setting SNR1 to %f\n", snr1);
#ifdef DEBUG_NR_DLSCHSIM
			printf("Setting SNR1 to %f\n", snr1);
#endif
			break;

		case 'p':
			extended_prefix_flag = 1;
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

		case 'y':
			n_tx = atoi(optarg);

			if ((n_tx == 0) || (n_tx > 2)) {
				printf("Unsupported number of TX antennas %d. Exiting.\n", n_tx);
				exit(-1);
			}

			break;

		case 'z':
			n_rx = atoi(optarg);

			if ((n_rx == 0) || (n_rx > 2)) {
				printf("Unsupported number of RX antennas %d. Exiting.\n", n_rx);
				exit(-1);
			}

			break;

	        case 'M':
      			SSB_positions = atoi(optarg);
			break;		    

		case 'N':
			Nid_cell = atoi(optarg);
			break;

		case 'R':
			N_RB_DL = atoi(optarg);
			break;

		case 'F':
			input_fd = fopen(optarg, "r");

			if (input_fd == NULL) {
				printf("Problem with filename %s. Exiting.\n", optarg);
				exit(-1);
			}

			break;

		case 'P':
			pbch_phase = atoi(optarg);

			if (pbch_phase > 3)
				printf("Illegal PBCH phase (0-3) got %d\n", pbch_phase);

			break;

		case 'L':
		  loglvl = atoi(optarg);
		  break;

		case 'm':
			Imcs = atoi(optarg);
			break;

		case 'l':
			nb_symb_sch = atoi(optarg);
			break;

		case 'r':
			nb_rb = atoi(optarg);
			break;

		case 'X':
		  strncpy(gNBthreads, optarg, sizeof(gNBthreads));
		  gNBthreads[sizeof(gNBthreads)-1]=0;
		  break;

		/*case 'x':
			transmission_mode = atoi(optarg);
			break;*/

		default:
		case 'h':
			printf("%s -h(elp) -p(extended_prefix) -N cell_id -f output_filename -F input_filename -g channel_model -n n_frames -t Delayspread -s snr0 -S snr1 -x transmission_mode -y TXant -z RXant -i Intefrence0 -j Interference1 -A interpolation_file -C(alibration offset dB) -N CellId\n", argv[0]);
			printf("-h This message\n");
			printf("-p Use extended prefix mode\n");
			printf("-V Enable VCD dumb functions\n");
			//printf("-d Use TDD\n");
			printf("-n Number of frames to simulate\n");
			printf("-s Starting SNR, runs from SNR0 to SNR0 + 5 dB.  If n_frames is 1 then just SNR is simulated\n");
			printf("-S Ending SNR, runs from SNR0 to SNR1\n");
			printf("-t Delay spread for multipath channel\n");
			printf("-g [A,B,C,D,E,F,G] Use 3GPP SCM (A,B,C,D) or 36-101 (E-EPA,F-EVA,G-ETU) models (ignores delay spread and Ricean factor)\n");
			//printf("-x Transmission mode (1,2,6 for the moment)\n");
			printf("-y Number of TX antennas used in eNB\n");
			printf("-z Number of RX antennas used in UE\n");
			//printf("-i Relative strength of first intefering eNB (in dB) - cell_id mod 3 = 1\n");
			//printf("-j Relative strength of second intefering eNB (in dB) - cell_id mod 3 = 2\n");
  		    printf("-M Multiple SSB positions in burst\n");
			printf("-N Nid_cell\n");
			printf("-R N_RB_DL\n");
			printf("-O oversampling factor (1,2,4,8,16)\n");
			printf("-A Interpolation_filname Run with Abstraction to generate Scatter plot using interpolation polynomial in file\n");
			//printf("-C Generate Calibration information for Abstraction (effective SNR adjustment to remove Pe bias w.r.t. AWGN)\n");
			//printf("-f Output filename (.txt format) for Pe/SNR results\n");
			printf("-F Input filename (.txt format) for RX conformance testing\n");
			printf("-d number of dlsch threads, 0: no dlsch parallelization\n");
			exit(-1);
			break;
		}
	}

	logInit();
	set_glog(loglvl);
	T_stdout = 1;

	if (snr1set == 0)
		snr1 = snr0 + 10;
	init_dlsch_tpool(dlsch_threads);

	if (ouput_vcd)
        vcd_signal_dumper_init("/tmp/openair_dump_nr_dlschsim.vcd");

	gNB2UE = new_channel_desc_scm(n_tx, n_rx, channel_model, 
				      61.44e6, //N_RB2sampling_rate(N_RB_DL),
				      40e6, //N_RB2channel_bandwidth(N_RB_DL),
                                      DS_TDL,
				      0, 0, 0, 0);

	if (gNB2UE == NULL) {
		printf("Problem generating channel model. Exiting.\n");
		exit(-1);
	}

	RC.gNB = (PHY_VARS_gNB **) malloc(sizeof(PHY_VARS_gNB *));
	RC.gNB[0] = calloc(1, sizeof(PHY_VARS_gNB));
	gNB = RC.gNB[0];
	gNB->threadPool = (tpool_t*)malloc(sizeof(tpool_t));
	initTpool(gNBthreads, gNB->threadPool, true);
	//gNB_config = &gNB->gNB_config;
	frame_parms = &gNB->frame_parms; //to be initialized I suppose (maybe not necessary for PBCH)
	frame_parms->nb_antennas_tx = n_tx;
	frame_parms->nb_antennas_rx = n_rx;
	frame_parms->N_RB_DL = N_RB_DL;
	frame_parms->Ncp = extended_prefix_flag ? EXTENDED : NORMAL;
	crcTableInit();
	nr_phy_config_request_sim(gNB, N_RB_DL, N_RB_DL, mu, Nid_cell,SSB_positions);
	phy_init_nr_gNB(gNB, 0, 1); //lowmem
	//init_eNB_afterRU();
	frame_length_complex_samples = frame_parms->samples_per_subframe;
	//frame_length_complex_samples_no_prefix = frame_parms->samples_per_subframe_wCP;
	s_re = malloc(2 * sizeof(double *));
	s_im = malloc(2 * sizeof(double *));
	r_re = malloc(2 * sizeof(double *));
	r_im = malloc(2 * sizeof(double *));
	txdata = malloc(2 * sizeof(int *));

	for (i = 0; i < 2; i++) {
		s_re[i] = malloc(frame_length_complex_samples * sizeof(double));
		bzero(s_re[i], frame_length_complex_samples * sizeof(double));
		s_im[i] = malloc(frame_length_complex_samples * sizeof(double));
		bzero(s_im[i], frame_length_complex_samples * sizeof(double));
		r_re[i] = malloc(frame_length_complex_samples * sizeof(double));
		bzero(r_re[i], frame_length_complex_samples * sizeof(double));
		r_im[i] = malloc(frame_length_complex_samples * sizeof(double));
		bzero(r_im[i], frame_length_complex_samples * sizeof(double));
		txdata[i] = malloc(frame_length_complex_samples * sizeof(int));
		bzero(r_re[i], frame_length_complex_samples * sizeof(int)); // [hna] r_re should be txdata
	}

	if (pbch_file_fd != NULL) {
		load_pbch_desc(pbch_file_fd);
	}

	//configure UE
	UE = malloc(sizeof(PHY_VARS_NR_UE));
	memcpy(&UE->frame_parms, frame_parms, sizeof(NR_DL_FRAME_PARMS));

	//phy_init_nr_top(frame_parms);
	if (init_nr_ue_signal(UE, 1) != 0) {
		printf("Error at UE NR initialisation\n");
		exit(-1);
	}

	//nr_init_frame_parms_ue(&UE->frame_parms);
	//init_nr_ue_transport(UE, 0);
        int num_codeword = NR_MAX_NB_LAYERS > 4? 2:1;
	for (int sf = 0; sf < 2; sf++) {
		for (i = 0; i < num_codeword; i++) {
			UE->dlsch[sf][0][i] = new_nr_ue_dlsch(Kmimo, 8, Nsoft, 5, N_RB_DL);
			if (!UE->dlsch[sf][0][i]) {
				printf("Can't get ue dlsch structures\n");
				exit(-1);
			}

			UE->dlsch[sf][0][i]->rnti = n_rnti;
		}
	}

	unsigned char harq_pid = 0; //dlsch->harq_ids[subframe];
        processingData_L1tx_t msgDataTx;
        init_DLSCH_struct(gNB, &msgDataTx);
	NR_gNB_DLSCH_t *dlsch = msgDataTx.dlsch[0][0];
	nfapi_nr_dl_tti_pdsch_pdu_rel15_t *rel15 = &dlsch->harq_process.pdsch_pdu.pdsch_pdu_rel15;
	//time_stats_t *rm_stats, *te_stats, *i_stats;
	uint8_t is_crnti = 0;
	unsigned int TBS = 8424;
	uint8_t nb_re_dmrs = 6;  // No data in dmrs symbol
	uint16_t length_dmrs = 1;
	uint8_t Nl = 1;
	uint8_t rvidx = 0;
	dlsch->rnti = 1;
	/*dlsch->harq_processes[0]->mcs = Imcs;
	 dlsch->harq_processes[0]->rvidx = rvidx;*/
	//printf("dlschsim harqid %d nb_rb %d, mscs %d\n",dlsch->harq_ids[subframe],
	//    dlsch->harq_processes[0]->nb_rb,dlsch->harq_processes[0]->mcs,dlsch->harq_processes[0]->Nl);
	unsigned char mod_order = nr_get_Qm_dl(Imcs, mcs_table);
        uint16_t rate = nr_get_code_rate_dl(Imcs, mcs_table);
	unsigned int available_bits = nr_get_G(nb_rb, nb_symb_sch, nb_re_dmrs, length_dmrs, mod_order, 1);
	TBS = nr_compute_tbs(mod_order,rate, nb_rb, nb_symb_sch, nb_re_dmrs*length_dmrs, 0, 0, Nl);
	printf("available bits %u TBS %u mod_order %d\n", available_bits, TBS, mod_order);
	//dlsch->harq_ids[subframe]= 0;
	rel15->rbSize         = nb_rb;
	rel15->NrOfSymbols    = nb_symb_sch;
	rel15->qamModOrder[0] = mod_order;
	rel15->nrOfLayers     = Nl;
	rel15->TBSize[0]      = TBS>>3;
        rel15->targetCodeRate[0] = rate;
        rel15->NrOfCodewords = 1;
        rel15->dmrsConfigType = NFAPI_NR_DMRS_TYPE1;
	rel15->dlDmrsSymbPos = 4;
	rel15->mcsIndex[0] = Imcs;
        rel15->numDmrsCdmGrpsNoData = 1;
        rel15->maintenance_parms_v3.tbSizeLbrmBytes = Tbslbrm;
	double modulated_input[16 * 68 * 384]; // [hna] 16 segments, 68*Zc
	short channel_output_fixed[16 * 68 * 384];
	//unsigned char *estimated_output;
	unsigned int errors_bit = 0;
	unsigned char test_input_bit[16 * 68 * 384];
	//estimated_output = (unsigned char *) malloc16(sizeof(unsigned char) * 16 * 68 * 384);
	unsigned char estimated_output_bit[16 * 68 * 384];
	NR_UE_DLSCH_t *dlsch0_ue = UE->dlsch[0][0][0];
	NR_DL_UE_HARQ_t *harq_process = dlsch0_ue->harq_processes[harq_pid];
	harq_process->mcs = Imcs;
	harq_process->mcs_table = mcs_table;
	harq_process->Nl = Nl;
	harq_process->nb_rb = nb_rb;
	harq_process->Qm = mod_order;
	harq_process->rvidx = rvidx;
	harq_process->R = rate;
        harq_process->TBS = TBS;
	harq_process->dmrsConfigType = NFAPI_NR_DMRS_TYPE1;
	harq_process->dlDmrsSymbPos = 4;
	harq_process->n_dmrs_cdm_groups = 1;
        harq_process->tbslbrm = Tbslbrm;
	printf("harq process ue mcs = %d Qm = %d, symb %d\n", harq_process->mcs, harq_process->Qm, nb_symb_sch);

	unsigned char *test_input=dlsch->harq_process.pdu;
	//unsigned char test_input[TBS / 8]  __attribute__ ((aligned(16)));
	for (i = 0; i < TBS / 8; i++)
		test_input[i] = (unsigned char) rand();

#ifdef DEBUG_NR_DLSCHSIM
	for (i = 0; i < TBS / 8; i++) printf("test_input[i]=%hhu \n",test_input[i]);
#endif

	/*for (int i=0; i<TBS/8; i++)
	 printf("test input[%d]=%d \n",i,test_input[i]);*/

	//printf("crc32: [0]->0x%08x\n",crc24c(test_input, 32));
	// generate signal
        unsigned char output[rel15->rbSize * NR_SYMBOLS_PER_SLOT * NR_NB_SC_PER_RB * 8 * NR_MAX_NB_LAYERS] __attribute__((aligned(32)));
        bzero(output,rel15->rbSize * NR_SYMBOLS_PER_SLOT * NR_NB_SC_PER_RB * 8 * NR_MAX_NB_LAYERS);
	if (input_fd == NULL) {
	  nr_dlsch_encoding(gNB, frame, slot, &dlsch->harq_process, frame_parms,output,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
	}

	for (SNR = snr0; SNR < snr1; SNR += snr_step) {
		n_errors = 0;
		n_false_positive = 0;

		for (trial = 0; trial < n_trials; trial++) {
			for (i = 0; i < available_bits; i++) {
#ifdef DEBUG_CODER
				if ((i&0xf)==0)
				printf("\ne %d..%d:    ",i,i+15);
#endif

				//if (i<16)
				//   printf("encoder output f[%d] = %d\n",i,dlsch->harq_processes[0]->f[i]);

				if (output[i] == 0)
					modulated_input[i] = 1.0;        ///sqrt(2);  //QPSK
				else
					modulated_input[i] = -1.0;        ///sqrt(2);

				//if (i<16) printf("modulated_input[%d] = %d\n",i,modulated_input[i]);
				//SNR =10;
				SNR_lin = pow(10, SNR / 10.0);
				sigma = 1.0 / sqrt(2 * SNR_lin);
				channel_output_fixed[i] = (short) quantize(sigma / 4.0 / 4.0,
						modulated_input[i] + sigma * gaussdouble(0.0, 1.0),
						qbits);
				//channel_output_fixed[i] = (char)quantize8bit(sigma/4.0,(2.0*modulated_input[i]) - 1.0 + sigma*gaussdouble(0.0,1.0));
				//printf("llr[%d]=%d\n",i,channel_output_fixed[i]);
				//printf("channel_output_fixed[%d]: %d\n",i,channel_output_fixed[i]);

				//channel_output_fixed[i] = (char)quantize(1,channel_output_fixed[i],qbits);
/*
				if (i<16)   printf("input[%d] %f => channel_output_fixed[%d] = %d\n",
						   i,modulated_input[i],
						   i,channel_output_fixed[i]);
*/
			}

#ifdef DEBUG_CODER
			printf("\n");
			exit(-1);
#endif

			vcd_signal_dumper_dump_function_by_name(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_DECODING0, VCD_FUNCTION_IN);

			ret = nr_dlsch_decoding(UE, &proc, 0, channel_output_fixed, &UE->frame_parms,
					dlsch0_ue, dlsch0_ue->harq_processes[0], frame, nb_symb_sch,
					slot,harq_pid, is_crnti);

			vcd_signal_dumper_dump_function_by_name(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_DECODING0, VCD_FUNCTION_OUT);

			if (ret > dlsch0_ue->max_ldpc_iterations)
				n_errors++;

			//count errors
			errors_bit = 0;

			for (i = 0; i < TBS; i++) {
				estimated_output_bit[i] = (dlsch0_ue->harq_processes[0]->b[i / 8] & (1 << (i & 7))) >> (i & 7);
				test_input_bit[i] = (test_input[i / 8] & (1 << (i & 7))) >> (i & 7); // Further correct for multiple segments

				if (estimated_output_bit[i] != test_input_bit[i]) {
					errors_bit++;
					//printf("estimated bits error occurs @%d ",i);
				}
			}

			if (errors_bit > 0) {
				n_false_positive++;
				if (n_trials == 1)
					printf("errors_bit %u (trial %d)\n", errors_bit, trial);
			}
		}

		printf("SNR %f, BLER %f (false positive %f)\n", SNR,
				(float) n_errors / (float) n_trials,
				(float) n_false_positive / (float) n_trials);

		if ((float) n_errors / (float) n_trials < target_error_rate) {
		  printf("PDSCH test OK\n");
		  break;
		}
	}

	/*LOG_M("txsigF0.m","txsF0", gNB->common_vars.txdataF[0],frame_length_complex_samples_no_prefix,1,1);
	 if (gNB->frame_parms.nb_antennas_tx>1)
	 LOG_M("txsigF1.m","txsF1", gNB->common_vars.txdataF[1],frame_length_complex_samples_no_prefix,1,1);*/

	//TODO: loop over slots
	/*for (aa=0; aa<gNB->frame_parms.nb_antennas_tx; aa++) {
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

	 LOG_M("txsig0.m","txs0", txdata[0],frame_length_complex_samples,1,1);
	 if (gNB->frame_parms.nb_antennas_tx>1)
	 LOG_M("txsig1.m","txs1", txdata[1],frame_length_complex_samples,1,1);


	 for (i=0; i<frame_length_complex_samples; i++) {
	 for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
	 r_re[aa][i] = ((double)(((short *)txdata[aa]))[(i<<1)]);
	 r_im[aa][i] = ((double)(((short *)txdata[aa]))[(i<<1)+1]);
	 }
	 }*/

  free_channel_desc_scm(gNB2UE);

  reset_DLSCH_struct(gNB, &msgDataTx);

  phy_free_nr_gNB(gNB);
  free(gNB->threadPool);
  free(RC.gNB[0]);
  free(RC.gNB);

  int num_cw = NR_MAX_NB_LAYERS > 4? 2:1;
  for (int sf = 0; sf < 2; sf++)
    for (int i = 0; i < num_cw; i++)
      free_nr_ue_dlsch(&UE->dlsch[sf][0][i], N_RB_DL);
  term_nr_ue_signal(UE, 1);
  free(UE);

	for (i = 0; i < 2; i++) {
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

	if (ouput_vcd)
        vcd_signal_dumper_close();

  loader_reset();
  logTerm();

	return (n_errors);
}

