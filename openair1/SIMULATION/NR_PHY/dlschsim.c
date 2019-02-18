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
#include "PHY/INIT/phy_init.h"
#include "PHY/NR_REFSIG/refsig_defs_ue.h"
#include "PHY/NR_REFSIG/nr_mod_table.h"
#include "PHY/MODULATION/modulation_eNB.h"
#include "PHY/MODULATION/modulation_UE.h"
#include "PHY/NR_TRANSPORT/nr_transport.h"
#include "PHY/NR_TRANSPORT/nr_dlsch.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"

#include "SCHED_NR/sched_nr.h"

//#include "PHY/MODULATION/modulation_common.h"
//#include "common/config/config_load_configmodule.h"
//#include "UTIL/LISTS/list.h"
//#include "common/ran_context.h"

//#define DEBUG_DLSCHSIM

PHY_VARS_gNB *gNB;
PHY_VARS_NR_UE *UE;
RAN_CONTEXT_t RC;

double cpuf;

// dummy functions
int nfapi_mode = 0;
int oai_nfapi_hi_dci0_req(nfapi_hi_dci0_request_t *hi_dci0_req) {
	return (0);
}
int oai_nfapi_tx_req(nfapi_tx_request_t *tx_req) {
	return (0);
}

int oai_nfapi_dl_config_req(nfapi_dl_config_request_t *dl_config_req) {
	return (0);
}

int oai_nfapi_ul_config_req(nfapi_ul_config_request_t *ul_config_req) {
	return (0);
}

int oai_nfapi_nr_dl_config_req(nfapi_nr_dl_config_request_t *dl_config_req) {
	return (0);
}

uint32_t from_nrarfcn(int nr_bandP, uint32_t dl_earfcn) {
	return (0);
}
int32_t get_uldl_offset(int eutra_bandP) {
	return (0);
}

NR_IF_Module_t *
NR_IF_Module_init(int Mod_id) {
	return (NULL);
}

void exit_function(const char *file, const char *function, const int line,
		const char *s) {
	const char *msg = s == NULL ? "no comment" : s;
	printf("Exiting at: %s:%d %s(), %s\n", file, line, function, msg);
	exit(-1);
}

// needed for some functions
PHY_VARS_NR_UE *PHY_vars_UE_g[1][1] = { { NULL } };
uint16_t n_rnti = 0x1234;

char quantize(double D, double x, unsigned char B) {
	double qxd;
	short maxlev;
	qxd = floor(x / D);
	maxlev = 1 << (B - 1); //(char)(pow(2,B-1));

	//printf("x=%f,qxd=%f,maxlev=%d\n",x,qxd, maxlev);

	if (qxd <= -maxlev)
		qxd = -maxlev;
	else if (qxd >= maxlev)
		qxd = maxlev - 1;

	return ((char) qxd);
}

int main(int argc, char **argv) {
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
	uint8_t write_output_file = 0;
	//  int subframe_offset;
	//  char fname[40], vname[40];
	int trial, n_trials = 1, n_errors = 0, n_false_positive = 0;
	uint8_t transmission_mode = 1, n_tx = 1, n_rx = 1;
	uint16_t Nid_cell = 0;
	channel_desc_t *gNB2UE;
	uint8_t extended_prefix_flag = 0;
	int8_t interf1 = -21, interf2 = -21;
	FILE *input_fd = NULL, *pbch_file_fd = NULL;
	//char input_val_str[50],input_val_str2[50];
	//uint16_t NB_RB=25;
	SCM_t channel_model = AWGN;  //Rayleigh1_anticorr;
	uint16_t N_RB_DL = 106, mu = 1;
	unsigned char frame_type = 0;
	unsigned char pbch_phase = 0;
	int frame = 0, subframe = 0;
	int frame_length_complex_samples;
	//int frame_length_complex_samples_no_prefix;
	NR_DL_FRAME_PARMS *frame_parms;
	//nfapi_nr_config_request_t *gNB_config;
	uint8_t Kmimo = 0;
	uint32_t Nsoft = 0;
	double sigma;
	unsigned char qbits = 8;
	int ret;
	//int run_initial_sync=0;
	int loglvl = OAILOG_WARNING;
	float target_error_rate = 0.01;

	uint16_t nb_symb_sch = 12;
	uint16_t nb_rb = 50;
	uint8_t Imcs = 9;

	cpuf = get_cpu_freq_GHz();

	if (load_configmodule(argc, argv) == 0) {
		exit_fun("[SOFTMODEM] Error, configuration module init failed\n");
	}

	//logInit();
	randominit(0);

	while ((c = getopt(argc, argv, "df:hpg:i:j:n:l:m:r:s:S:y:z:N:F:R:P:")) != -1) {
		switch (c) {
		case 'f':
			write_output_file = 1;
			output_fd = fopen(optarg, "w");

			if (output_fd == NULL) {
				printf("Error opening %s\n", optarg);
				exit(-1);
			}

			break;

		case 'd':
			frame_type = 1;
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
				msg("Unsupported channel model!\n");
				exit(-1);
			}

			break;

		case 'i':
			interf1 = atoi(optarg);
			break;

		case 'j':
			interf2 = atoi(optarg);
			break;

		case 'n':
			n_trials = atoi(optarg);
			break;

		case 's':
			snr0 = atof(optarg);
			msg("Setting SNR0 to %f\n", snr0);
			break;

		case 'S':
			snr1 = atof(optarg);
			snr1set = 1;
			msg("Setting SNR1 to %f\n", snr1);
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
				msg("Unsupported number of tx antennas %d\n", n_tx);
				exit(-1);
			}

			break;

		case 'z':
			n_rx = atoi(optarg);

			if ((n_rx == 0) || (n_rx > 2)) {
				msg("Unsupported number of rx antennas %d\n", n_rx);
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
			input_fd = fopen(optarg, "r");

			if (input_fd == NULL) {
				printf("Problem with filename %s\n", optarg);
				exit(-1);
			}

			break;

		case 'P':
			pbch_phase = atoi(optarg);

			if (pbch_phase > 3)
				printf("Illegal PBCH phase (0-3) got %d\n", pbch_phase);

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

		case 'x':
			transmission_mode = atoi(optarg);
			break;

		default:
		case 'h':
			printf("%s -h(elp) -p(extended_prefix) -N cell_id -f output_filename -F input_filename -g channel_model -n n_frames -t Delayspread -s snr0 -S snr1 -x transmission_mode -y TXant -z RXant -i Intefrence0 -j Interference1 -A interpolation_file -C(alibration offset dB) -N CellId\n", argv[0]);
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
			printf("-N Nid_cell\n");
			printf("-R N_RB_DL\n");
			printf("-O oversampling factor (1,2,4,8,16)\n");
			printf("-A Interpolation_filname Run with Abstraction to generate Scatter plot using interpolation polynomial in file\n");
			//    printf("-C Generate Calibration information for Abstraction (effective SNR adjustment to remove Pe bias w.r.t. AWGN)\n");
			printf("-f Output filename (.txt format) for Pe/SNR results\n");
			printf("-F Input filename (.txt format) for RX conformance testing\n");
			exit(-1);
			break;
		}
	}

	logInit();
	set_glog(loglvl);
	T_stdout = 1;

	if (snr1set == 0)
		snr1 = snr0 + 10;

	gNB2UE = new_channel_desc_scm(n_tx, n_rx, channel_model, 
				      61.44e6, //N_RB2sampling_rate(N_RB_DL),
				      40e6, //N_RB2channel_bandwidth(N_RB_DL),
				      0, 0, 0);

	if (gNB2UE == NULL) {
		msg("Problem generating channel model. Exiting.\n");
		exit(-1);
	}

	RC.gNB = (PHY_VARS_gNB ** *) malloc(sizeof(PHY_VARS_gNB **));
	RC.gNB[0] = (PHY_VARS_gNB **) malloc(sizeof(PHY_VARS_gNB *));
	RC.gNB[0][0] = malloc(sizeof(PHY_VARS_gNB));
	gNB = RC.gNB[0][0];
	//gNB_config = &gNB->gNB_config;
	frame_parms = &gNB->frame_parms; //to be initialized I suppose (maybe not necessary for PBCH)
	frame_parms->nb_antennas_tx = n_tx;
	frame_parms->nb_antennas_rx = n_rx;
	frame_parms->N_RB_DL = N_RB_DL;
	frame_parms->Ncp = extended_prefix_flag ? EXTENDED : NORMAL;
	crcTableInit();
	nr_phy_config_request_sim(gNB, N_RB_DL, N_RB_DL, mu, Nid_cell);
	phy_init_nr_gNB(gNB, 0, 0);
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
		bzero(r_re[i], frame_length_complex_samples * sizeof(int));
	}

	if (pbch_file_fd != NULL) {
		load_pbch_desc(pbch_file_fd);
	}

	/*  for (int k=0; k<2; k++) {
	 // Create transport channel structures for 2 transport blocks (MIMO)
	 for (i=0; i<2; i++) {
	 gNB->dlsch[k][i] = new_gNB_dlsch(Kmimo,8,Nsoft,0,frame_parms,gNB_config);

	 if (!gNB->dlsch[k][i]) {
	 printf("Can't get eNB dlsch structures\n");
	 exit(-1);
	 }
	 gNB->dlsch[k][i]->Nsoft = 10;
	 gNB->dlsch[k][i]->rnti = n_rnti+k;
	 }
	 }*/
	//configure UE
	UE = malloc(sizeof(PHY_VARS_NR_UE));
	memcpy(&UE->frame_parms, frame_parms, sizeof(NR_DL_FRAME_PARMS));

	//phy_init_nr_top(frame_parms);
	if (init_nr_ue_signal(UE, 1, 0) != 0) {
		printf("Error at UE NR initialisation\n");
		exit(-1);
	}

	//nr_init_frame_parms_ue(&UE->frame_parms);
	//init_nr_ue_transport(UE, 0);
	for (int sf = 0; sf < 2; sf++) {
		for (i = 0; i < 2; i++) {
			UE->dlsch[sf][0][i] = new_nr_ue_dlsch(Kmimo, 8, Nsoft, 5, N_RB_DL,
					0);

			if (!UE->dlsch[sf][0][i]) {
				printf("Can't get ue dlsch structures\n");
				exit(-1);
			}

			UE->dlsch[sf][0][i]->rnti = n_rnti;
		}
	}

	UE->dlsch_SI[0] = new_nr_ue_dlsch(1, 1, Nsoft, 5, N_RB_DL, 0);
	UE->dlsch_ra[0] = new_nr_ue_dlsch(1, 1, Nsoft, 5, N_RB_DL, 0);
	unsigned char harq_pid = 0; //dlsch->harq_ids[subframe];
	NR_gNB_DLSCH_t *dlsch = gNB->dlsch[0][0];
	nfapi_nr_dl_config_dlsch_pdu_rel15_t *rel15 = &dlsch->harq_processes[harq_pid]->dlsch_pdu.dlsch_pdu_rel15;
	//time_stats_t *rm_stats, *te_stats, *i_stats;
	uint8_t is_crnti = 0, llr8_flag = 0;
	unsigned int TBS = 8424;
	unsigned int available_bits;
	uint8_t nb_re_dmrs = 6;
	uint16_t length_dmrs = 1;
	unsigned char mod_order;
	uint8_t Nl = 1;
	uint8_t rvidx = 0;
	dlsch->rnti = 1;
	/*dlsch->harq_processes[0]->mcs = Imcs;
	 dlsch->harq_processes[0]->rvidx = rvidx;*/
	//printf("dlschsim harqid %d nb_rb %d, mscs %d\n",dlsch->harq_ids[subframe],
	//    dlsch->harq_processes[0]->nb_rb,dlsch->harq_processes[0]->mcs,dlsch->harq_processes[0]->Nl);
	mod_order = nr_get_Qm(Imcs, 1);
	available_bits = nr_get_G(nb_rb, nb_symb_sch, nb_re_dmrs, length_dmrs, mod_order, 1);
	TBS = nr_compute_tbs(Imcs, nb_rb, nb_symb_sch, nb_re_dmrs, length_dmrs, Nl);
	printf("available bits %d TBS %d mod_order %d\n", available_bits, TBS, mod_order);
	//dlsch->harq_ids[subframe]= 0;
	rel15->n_prb = nb_rb;
	rel15->nb_symbols = nb_symb_sch;
	rel15->modulation_order = mod_order;
	rel15->nb_layers = Nl;
	rel15->nb_re_dmrs = nb_re_dmrs;
	rel15->transport_block_size = TBS;
	double *modulated_input = malloc16(sizeof(double) * 16 * 68 * 384);
	short *channel_output_fixed = malloc16(sizeof(short) * 16 * 68 * 384);
	short *channel_output_uncoded = malloc16(sizeof(unsigned short) * 16 * 68 * 384);
	double errors_bit_uncoded = 0;
	unsigned char *estimated_output;
	unsigned char *estimated_output_bit;
	unsigned char *test_input_bit;
	unsigned int errors_bit = 0;
	test_input_bit = (unsigned char *) malloc16(sizeof(unsigned char) * 16 * 68 * 384);
	estimated_output = (unsigned char *) malloc16(sizeof(unsigned char) * 16 * 68 * 384);
	estimated_output_bit = (unsigned char *) malloc16(sizeof(unsigned char) * 16 * 68 * 384);
	NR_UE_DLSCH_t *dlsch0_ue = UE->dlsch[0][0][0];
	NR_DL_UE_HARQ_t *harq_process = dlsch0_ue->harq_processes[harq_pid];
	harq_process->mcs = Imcs;
	harq_process->Nl = Nl;
	harq_process->nb_rb = nb_rb;
	harq_process->Qm = mod_order;
	harq_process->rvidx = rvidx;
	printf("harq process ue mcs = %d Qm = %d, symb %d\n", harq_process->mcs, harq_process->Qm, nb_symb_sch);
	unsigned char *test_input;
	test_input = (unsigned char *) malloc16(sizeof(unsigned char) * TBS / 8);

	for (i = 0; i < TBS / 8; i++)
		test_input[i] = (unsigned char) rand();

	estimated_output = harq_process->b;

#ifdef DEBUG_DLSCHSIM
	for (i = 0; i < TBS / 8; i++) printf("test_input[i]=%d \n",test_input[i]);
#endif

	/*for (int i=0; i<TBS/8; i++)
	 printf("test input[%d]=%d \n",i,test_input[i]);*/

	//printf("crc32: [0]->0x%08x\n",crc24c(test_input, 32));
	// generate signal
	if (input_fd == NULL) {
		nr_dlsch_encoding(test_input, subframe, dlsch, frame_parms);
	}

	for (SNR = snr0; SNR < snr1; SNR += snr_step) {
		n_errors = 0;
		n_false_positive = 0;

		for (trial = 0; trial < n_trials; trial++) {
			errors_bit_uncoded = 0;
			for (i = 0; i < available_bits; i++) {
#ifdef DEBUG_CODER
				if ((i&0xf)==0)
				printf("\ne %d..%d:    ",i,i+15);
#endif

				//if (i<16)
				//   printf("encoder output f[%d] = %d\n",i,dlsch->harq_processes[0]->f[i]);
				if (dlsch->harq_processes[0]->f[i] == 0)
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

				//if (i<16)   printf("channel_output_fixed[%d] = %d\n",i,channel_output_fixed[i]);

				//Uncoded BER
				if (channel_output_fixed[i] < 0)
					channel_output_uncoded[i] = 1;  //QPSK demod
				else
					channel_output_uncoded[i] = 0;

				if (channel_output_uncoded[i] != dlsch->harq_processes[harq_pid]->f[i])
					errors_bit_uncoded = errors_bit_uncoded + 1;
			}

			//if (errors_bit_uncoded>10)
			//printf("errors bits uncoded %f\n", errors_bit_uncoded);
#ifdef DEBUG_CODER
			printf("\n");
			exit(-1);
#endif
			ret = nr_dlsch_decoding(UE, channel_output_fixed, &UE->frame_parms,
					dlsch0_ue, dlsch0_ue->harq_processes[0], frame, nb_symb_sch,
					subframe, harq_pid, is_crnti, llr8_flag);

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
					printf("errors_bit %d (trial %d)\n", errors_bit, trial);
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

	for (i = 0; i < 2; i++) {
		printf("gNB %d\n", i);
		free_gNB_dlsch(gNB->dlsch[0][i]);
		printf("UE %d\n", i);
		free_nr_ue_dlsch(UE->dlsch[0][0][i]);
	}

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

	return (n_errors);
}

